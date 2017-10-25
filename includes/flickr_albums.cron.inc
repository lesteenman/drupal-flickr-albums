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
      watchdog('flickr_albums', 'Already found an album node; Last update was ' . $album_node->field_flickr_albums_date_update . ', web album update is ' . $album['date_update']);
      if ($album_node->field_flickr_albums_date_update >= $album['date_update']) continue;
    }
    else {
      // Create a new basic album
      /* $album_node = entity_create('node', array( */
      /*   'type' => ALBUM_NODE_TYPE, */
      /* )); */

      $album_node = new stdClass();
      $album_node->type = ALBUM_NODE_TYPE;
      node_object_prepare($album_node);
    }

    $albumsDone++;

    $album_info = $flickr->photosets_getInfo($album['id']);

    /* $node_wrapper = entity_metadata_wrapper('node', $album_node); */
    /* $node_wrapper->title->set($album_info['title']['_content']); */
    /* $node_wrapper->field_flickr_albums_flickr_id->set($album['id']); */
    /* $node_wrapper->field_flickr_albums_secret->set($album['secret']); */
    /* $node_wrapper->field_flickr_albums_primary_photo->set($album['primary']); */
    /* $node_wrapper->field_flickr_albums_server->set($album['server']); */
    /* $node_wrapper->field_flickr_albums_farm->set($album['farm']); */
    /* $node_wrapper->field_flickr_albums_count_photos->set($album['photos']); */
    /* $node_wrapper->field_flickr_albums_count_videos->set($album['videos']); */
    /* $node_wrapper->field_flickr_albums_date_update->set($album['date_update']); */
    /* $node_wrapper->save(); */

    // Make sure we're up to date on all changes to the album.
    $album_node->title = $album_info['title']['_content'];
    $album_node->field_flickr_albums_flickr_id = $album['id'];
    $album_node->field_flickr_albums_secret = $album['secret'];
    $album_node->field_flickr_albums_primary_photo = $album['primary'];
    $album_node->field_flickr_albums_server = $album['server'];
    $album_node->field_flickr_albums_farm = $album['farm'];
    $album_node->field_flickr_albums_count_photos = $album['photos'];
    $album_node->field_flickr_albums_count_videos = $album['videos'];
    $album_node->field_flickr_albums_date_update = $album['date_update'];
    $album_node = node_submit($album_node);
    node_save($album_node);

    // Add photos (if we're not yet past the set batchsize)
    $photos = $flickr->photosets_getPhotos($album['id'])['photoset']['photo'];
    watchdog('flickr_albums', 'Also found ' . count($photos) . ' photos');
    /* foreach ($photos as $photo) { */
    /*   if ($photosDone === $count) break; */

    /*   $photo_node = flickr_albums_api_get_photo($album['id'], $photo['id']); */
    /*   if ($photo_node) { */
    /*     if (!$photo_node->field_flickr_albums_date_update >= $photo['date_update']) continue; */
    /*   } */
    /*   else { */
    /*     // Create a new basic photo */
    /*     $photo_node = new stdClass(); */
    /*     $photo_node->type = PHOTO_NODE_TYPE; */
    /*     node_object_prepare($photo_node); */
    /*   } */

    /*   $photo_node->title = $photo['title']; */
    /*   $photo_node->field_flickr_albums_flickr_id = $photo['id']; */
    /*   $photo_node->field_flickr_albums_secret = $photo['secret']; */
    /*   $photo_node->field_flickr_albums_server = $photo['server']; */
    /*   $photo_node->field_flickr_albums_farm = $photo['farm']; */
    /*   $photo_node->field_flickr_albums_visibility_public = $photo['ispublic'] ? 1 : 0; */
    /*   $photo_node->field_flickr_albums_visibility_friends = $photo['isfriend'] ? 1 : 0; */
    /*   $photo_node->field_flickr_albums_visibility_family = $photo['isfamily'] ? 1 : 0; */
    /*   $photo_node->field_flickr_albums_date_update = $photo['date_update']; */
    /*   node_save($photo_node); */

    /*   // As we still want to make sure we have the weight of all albums correct, we continue for */
    /*   // just the albums. */
    /*   if ($photosDone++ === $count) break; */
    /* } */

    break;
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