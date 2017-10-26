<?php

module_load_include('module', 'flickrapi');
module_load_include('inc', 'flickr_albums', 'includes/flickr_albums.api');

/**
 * Updates $count photos. -1 means it will update all photos.
 */
function flickr_albums_process_batch($count) {
  $flickr = flickrapi_phpFlickr();
  $token = variable_get('flickr_albums_token', null);

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

  // Update all albums
  $photosDone = 0;
  $albumsDone = 0;
  foreach ($albums as $album) {
    $album_node = flickr_albums_api_get_album($album['id']);

    watchdog('flickr_albums', 'found album for ' . $album['id'] . ': ' . json_encode($album_node));

    if ($album_node) {
      // Album exists, test if it has changed
      $album_wrapper = entity_metadata_wrapper('node', $album_node);
      if ($album_wrapper->flickr_albums_date_update->value() >= $album['date_update']) {
        watchdog('flickr_albums', 'Skipping album ' . $album['id'] . ', since there are no updates');
        continue;
      }
    }
    else {
      // Create a new basic album
      $album_node = entity_create('node', array(
        'type' => ALBUM_NODE_TYPE,
      ));
      $album_wrapper = entity_metadata_wrapper('node', $album_node);

      /* $album_node = new stdClass(); */
      /* $album_node->type = ALBUM_NODE_TYPE; */
      /* node_object_prepare($album_node); */
    }

    $albumsDone++;

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
    $album_wrapper->flickr_albums_weight->set($albumsDone);
    $album_wrapper->save();

    // Add photos (if we're not yet past the set batchsize)
    $photos = $flickr->photosets_getPhotos($album['id'])['photoset']['photo'];
    watchdog('flickr_albums', 'Also found ' . count($photos) . ' photos');

    $photoWeight = 0;
    foreach ($photos as $photo) {
      if ($photosDone === $count) break;

      $photo_node = flickr_albums_api_get_photo($album['id'], $photo['id']);
      if ($photo_node) {
        $photo_wrapper = entity_metadata_wrapper('node', $photo_node);
        if (!$photo_wrapper->flickr_albums_date_update->value() >= $photo['date_update']) continue;
      }
      else {
        // Create a new basic photo
        $photo_node = entity_create('node', array(
          'type' => PHOTO_NODE_TYPE,
        ));
        $photo_wrapper = entity_metadata_wrapper('node', $photo_node);
      }

      $photo_info = $flickr->photos_getInfo($photo['id'])['photo'];

      $photo_wrapper->title->set($photo['title']);
      $photo_wrapper->flickr_albums_flickr_id->set($photo['id']);
      $photo_wrapper->flickr_albums_secret->set($photo['secret']);
      $photo_wrapper->flickr_albums_original_secret->set($photo_info['originalsecret']);
      $photo_wrapper->flickr_albums_server->set($photo['server']);
      $photo_wrapper->flickr_albums_farm->set($photo['farm']);
      $photo_wrapper->flickr_albums_original_format->set($photo_info['originalformat']);
      $photo_wrapper->flickr_albums_media->set($photo_info['media']);
      $photo_wrapper->flickr_albums_album_id->set($album['id']);
      $photo_wrapper->flickr_albums_visibility_public->set($photo['ispublic'] ? 1 : 0);
      $photo_wrapper->flickr_albums_visibility_friends->set($photo['isfriend'] ? 1 : 0);
      $photo_wrapper->flickr_albums_visibility_family->set($photo['isfamily'] ? 1 : 0);
      $photo_wrapper->flickr_albums_date_update->set($photo_info['dates']['lastupdate']);
      $photo_wrapper->flickr_albums_date_taken->set($photo_info['dates']['lastupdate']);
      $photo_wrapper->flickr_albums_date_upload->set($photo_info['dates']['lastupdate']);
      $photo_wrapper->flickr_albums_weight->set($photoWeight++);
      $photo_wrapper->save();

      // As we still want to make sure we have the weight of all albums correct, we continue for
      // just the albums.
      if ($photosDone++ === $count) break;
    }
  }

  watchdog('flickr_albums', "cron run processed $photosDone photos and $albumsDone (all) albums for user $userId.");
  drupal_set_message("processed $photosDone photos and $albumsDone (all) albums");

  // Check if we have the correct permissions (read?)

  // At any point, once we reach $count photos: return. The next iteration will automatically continue.
  // Get all photosets
  // For all new photosets:
  // - Create photoset
  // - Import up to $count photos, in batches of 500 (Flickr max)
  // For all existing photosets:
  // - Check if the 'updated' date is later than our last node update
  //
  // For all photos we import:
  // - Save all relevant fields
}
