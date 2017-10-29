<?php

module_load_include('module', 'flickrapi');
module_load_include('inc', 'flickr_albums', 'includes/flickr_albums.api');

/**
 * Updates info on all albums. Does not actually update any albums.
 * Can be used when changing userId to find the number of albums and photos.
 */
function flickr_albums_update_album_info() {
}

/**
 * Updates $count photos. -1 means it will update all photos.
 */
function flickr_albums_process_batch($batch_size) {
  $flickr = flickrapi_phpFlickr();
  $token = variable_get('flickr_albums_token', null);

  // TODO: Check if we have the correct permissions (read?)

  $userId = variable_get('flickr_albums_userId', null);
  if (!$userId) {
    watchdog('flickr_albums', 'cron could not fetch Flickr albums, as no userId has been given.');
    return;
  }

  watchdog('flickr_albums', 'starting cron run for Flickr user ' . $userId);

  // Fetch basic information about all albums
  $result = false;
  $albums = [];
  while (!$result || $result['page'] < $result['pages']) {
    $page = $result ? $result['page'] + 1 : 1;
    $result = $flickr->photosets_getList($userId, $page);

    if ($result['pages'] === 0) {
      watchdog('flickr_albums', 'No albums found');
      return;
    }

    watchdog('flickr_albums', 'Fetching page ' . $page . ' out of ' . $result['pages']);
    $albums += $result['photoset'];
  }

  watchdog('flickr_albums', 'cron importing ' . count($albums) . ' albums');

  $photos_total = 0; // Total photos found
  $videos_total = 0; // Total photos found
  $albums_total = 0; // Total albums found

  $photos_synced = 0; // Photos synced total, including previous batches
  $videos_synced = 0; // Photos synced total, including previous batches
  $albums_synced = 0;

  $media_done = 0; // All photos/videos that have been processed this batch
  $videos_done = 0; // Videos that have been processed this batch
  $photos_done = 0; // Photos that have been processed this batch
  $albums_done = 0; // Albums that have been updated this batch

  // Update all albums
  foreach ($albums as $album) {
    $albums_total++;
    $album_node = flickr_albums_api_get_album($album['id']);

    // Queries used to see the amount of photos/videos we already have for this album
    $videos_synced_query = new EntityFieldQuery();
    $videos_synced_query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', PHOTO_NODE_TYPE)
      ->fieldCondition('flickr_albums_album_id', 'value', $album['id']);
    $videos_synced_query->fieldCondition('flickr_albums_media', 'value', 'video');
    $album_videos_synced = $videos_synced_query->count()->execute();

    $photos_synced_query = new EntityFieldQuery();
    $photos_synced_query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', PHOTO_NODE_TYPE)
      ->fieldCondition('flickr_albums_album_id', 'value', $album['id']);
    $photos_synced_query->fieldCondition('flickr_albums_media', 'value', 'photo');
    $album_photos_synced = $photos_synced_query->count()->execute();

    $videos_synced += $album_videos_synced;
    $photos_synced += $album_photos_synced;

    if ($album_node) {
      // Album exists, test if it has changed
      $album_wrapper = entity_metadata_wrapper('node', $album_node);

      $album_photos_total = $album_wrapper->flickr_albums_count_photos->value();
      $album_videos_total = $album_wrapper->flickr_albums_count_videos->value();
      $photos_total += $album_photos_total;
      $videos_total += $album_videos_total;

      watchdog('flickr_albums', "Album {$album['id']} has $album_photos_synced/$album_photos_total photos and $album_videos_synced/$album_videos_total videos");

      // TODO: Check if we've synced all photos/videos for this album yet!

      if ($album_wrapper->flickr_albums_date_update->value() >= $album['date_update'] &&
          $album_photos_synced === $album_photos_total &&
          $album_videos_synced === $album_videos_total) {
        watchdog('flickr_albums', 'Skipping album ' . $album['id'] . ', since there are no updates');
        $albums_synced++;
        continue;
      }
    }
    else {
      // Create a new basic album
      $album_node = entity_create('node', array(
        'type' => ALBUM_NODE_TYPE,
      ));
      $album_wrapper = entity_metadata_wrapper('node', $album_node);
    }

    $albums_done++;
    $albums_synced++;

    $album_info = $flickr->photosets_getInfo($album['id']);

    // Make sure we're up to date on all changes to the album.
    $album_wrapper->title->set($album_info['title']['_content']);
    $album_wrapper->flickr_albums_description->set($album_info['description']['_content']);
    $album_wrapper->flickr_albums_flickr_id->set($album['id']);
    $album_wrapper->flickr_albums_secret->set($album['secret']);
    $album_wrapper->flickr_albums_primary_photo->set($album['primary']);
    $album_wrapper->flickr_albums_server->set($album['server']);
    $album_wrapper->flickr_albums_farm->set($album['farm']);
    $album_wrapper->flickr_albums_count_photos->set($album['photos']);
    $album_wrapper->flickr_albums_count_videos->set($album['videos']);
    $album_wrapper->flickr_albums_date_update->set($album['date_update']);
    $album_wrapper->flickr_albums_date_create->set($album['date_create']);
    $album_wrapper->flickr_albums_weight->set($albums_total); // TODO: Verify and fix if necessary!
    $album_wrapper->save();

    // Now get photos for this album, if we're not yet over the limit
    if ($media_done >= $batch_size - 1) continue;

    $extras = 'url_o,original_format,last_update,date_taken,date_upload,media';
    $photos = $flickr->photosets_getPhotos($album['id'], $extras)['photoset']['photo'];

    $photoWeight = 0;
    foreach ($photos as $photo) {
      $photo_node = flickr_albums_api_get_photo($album['id'], $photo['id']);
      $photoWeight++; // TODO: Verify and fix!

      if ($photo_node) {
        // Already exist, check if it is still up-to-date
        $photo_wrapper = entity_metadata_wrapper('node', $photo_node);
        if ($photo['lastupdate'] <= $photo_wrapper->flickr_albums_date_update->value()) {
          /* if ($photo_wrapper->flickr_albums_media->value() === 'video') $videos_synced++; */
          /* else $photos_synced++; */
          continue;
        }
      } else {
        // Create a new photo
        $photo_node = entity_create('node', array(
          'type' => PHOTO_NODE_TYPE,
        ));
        $photo_wrapper = entity_metadata_wrapper('node', $photo_node);
      }

      /* $photo_info = $flickr->photos_getInfo($photo['id'])['photo']; */

      /* dpm(['photo' => $photo, 'node' => $photo_node]); */
      
      preg_match('/[0-9]+_([0-9a-zA-Z]+)_o.jpg/', $photo['url_o'], $matches);
      $original_secret = $matches[1];

      $photo_wrapper->title->set($photo['title']);
      $photo_wrapper->flickr_albums_flickr_id->set($photo['id']);
      $photo_wrapper->flickr_albums_secret->set($photo['secret']);
      $photo_wrapper->flickr_albums_original_secret->set($original_secret);
      $photo_wrapper->flickr_albums_server->set($photo['server']);
      $photo_wrapper->flickr_albums_farm->set($photo['farm']);
      $photo_wrapper->flickr_albums_original_format->set($photo['originalformat']);
      $photo_wrapper->flickr_albums_media->set($photo['media']);
      $photo_wrapper->flickr_albums_album_id->set($album['id']);
      $photo_wrapper->flickr_albums_visibility_public->set($photo['ispublic'] ? 1 : 0);
      $photo_wrapper->flickr_albums_visibility_friends->set($photo['isfriend'] ? 1 : 0);
      $photo_wrapper->flickr_albums_visibility_family->set($photo['isfamily'] ? 1 : 0);
      $photo_wrapper->flickr_albums_date_update->set($photo['lastupdate']);
      $photo_wrapper->flickr_albums_date_taken->set(strtotime($photo['datetaken']));
      $photo_wrapper->flickr_albums_date_upload->set($photo['dateupload']);
      $photo_wrapper->flickr_albums_weight->set($photoWeight);
      $photo_wrapper->save();

      if ($photo_wrapper->flickr_albums_media->value() === 'video') {
        $videos_done++;
        $videos_synced++;
      }
      else {
        $photos_done++;
        $photos_synced++;
      }

      if ($media_done++ >= $batch_size - 1) break;
    }
  }

  watchdog('flickr_albums', "cron run processed $photos_done photos and $albums_done/$albums_total albums for user $userId.");
  drupal_set_message("processed $photos_done/$photos_total photos and $videos_done/$videos_total videos. Information about $albums_done/$albums_total albums has also been synchronized. Total synchronized: $albums_synced/$albums_total albums, $photos_synced/$photos_total photos and $videos_synced/$videos_total videos.");

  variable_set('flickr_albums_albums_total', $albums_total);
  variable_set('flickr_albums_photos_total', $photos_total);
  variable_set('flickr_albums_videos_total', $videos_total);
  variable_set('flickr_albums_albums_synced', $albums_synced);
  variable_set('flickr_albums_photos_synced', $photos_synced);
  variable_set('flickr_albums_videos_synced', $videos_synced);

  variable_set('flickr_albums_last_sync', time());
}
