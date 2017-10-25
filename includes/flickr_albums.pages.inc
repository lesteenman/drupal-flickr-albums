<?php

module_load_include('module', 'flickrapi'); // TODO: Should not be necessary once we get things using cron!
module_load_include('inc', 'flickr_albums', 'includes/flickr_albums.api');

function flickr_albums_list() {
  $albums = array();

  $flickr = flickrapi_phpFlickr();
  $token = variable_get('flickr_albums_token', null);
  $userId = variable_get('flickr_albums_userId', null);
  if ($userId) {
    $albums = $flickr->photosets_getList($userId);
  }

  // TODO: Get albums from the database. Based on the unique albumIds of the photo IDs.

  $build = array(
    '#theme' => 'flickr_albums_list',
    '#description' => t('List of albums'),
    '#albums' => $albums,
  );

  dpm(['albums' => $albums]);

  return drupal_render($build);
}

function flickr_albums_album($album_id) {
  $photos = array();

  // TODO: Get photos from the database. Based on the unique albumIds of the photo IDs.

  $build = array(
    '#theme' => 'flickr_albums_album',
    '#description' => t('Flickr Album'),
    '#photos' => $photos,
  );

  dpm(['photos' => $photos]);

  return drupal_render($build);
}
