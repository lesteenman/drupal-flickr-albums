<?php

module_load_include('module', 'flickrapi'); // TODO: Should not be necessary once we get things using cron!
module_load_include('inc', 'flickr_albums', 'includes/flickr_albums.api');

function flickr_albums_list() {
  $albums = array();

  /* $flickr = flickrapi_phpFlickr(); */

  /* $token = variable_get('flickr_albums_token', null); */
  /* if (!empty($token)) { */
  /*   $flickr->setToken($token); */
  /* } */

  /* $userId = variable_get('flickr_albums_userId', '11467783@N05'); */
  /* if ($userId) { */
  /*   $albums = $flickr->photosets_getList($userId); */
  /* } */

  /* $album_info = $flickr->photosets_getInfo($albums['photoset'][0]['id']); */
  /* $photos = $flickr->photosets_getPhotos($albums['photoset'][0]['id']); */
  /* $photo_info = $flickr->photos_getInfo($photos['photoset']['photo'][0]['id']); */

  /* // TODO: Get albums from the database. Based on the unique albumIds of the photo IDs. */

  /* dpm(['albums' => $albums, 'photos' => $photos, 'album info' => $album_info, 'photo info' => $photo_info]); */

  drupal_add_css(drupal_get_path('module', 'flickr_albums') . '/style.css');

  $albums = array();
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', ALBUM_NODE_TYPE)
    ->fieldOrderBy('flickr_albums_weight', 'value', 'asc');
  $result = $query->execute();

  if (!empty($result['node'])) {
    foreach ($result['node'] as $nid => $data) {
      $album_node = node_load($nid);
      $view = node_view($album_node, 'teaser');
      $albums[] = drupal_render($view);
    }
  }

  $build = array(
    '#theme' => 'flickr_albums_list',
    '#description' => t('List of albums'),
    '#albums' => $albums,
  );

  return drupal_render($build);
}

function flickr_albums_preprocess_photo(&$variables, $hook) {
  $photo_node = $variables['node'];
  $photo_wrapper = entity_metadata_wrapper('node', $photo_node);

  drupal_add_css(drupal_get_path('module', 'flickr_albums') . '/style.css');

  $variables['urls'] = flickr_albums_photo_urls($photo_wrapper);
}

function flickr_albums_preprocess_album_teaser(&$variables, $hook) {
  $album_node = $variables['node'];
  $album_wrapper = entity_metadata_wrapper('node', $album_node);

  drupal_add_css(drupal_get_path('module', 'flickr_albums') . '/style.css');

  $primary = $album_wrapper->flickr_albums_primary_photo->value();
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', PHOTO_NODE_TYPE)
    ->fieldCondition('flickr_albums_flickr_id', 'value', $primary);
  $result = $query->execute();

  if (!empty($result['node'])) {
    $nid = array_keys($result['node'])[0];
    $photo_node = node_load($nid);
    $photo_wrapper = entity_metadata_wrapper('node', $photo_node);
    $urls = flickr_albums_photo_urls($photo_wrapper);
    $variables['primary'] = $urls;
  }
}

function flickr_albums_preprocess_album(&$variables, $hook) {
  $album_node = $variables['node'];
  $album_wrapper = entity_metadata_wrapper('node', $album_node);

  drupal_add_css(drupal_get_path('module', 'flickr_albums') . '/style.css');

  $photos = array();
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', PHOTO_NODE_TYPE)
    ->fieldOrderBy('flickr_albums_weight', 'value', 'asc')
    ->fieldCondition('flickr_albums_album_id', 'value', $album_wrapper->flickr_albums_flickr_id->value());
  $result = $query->execute();

  if (!empty($result['node'])) {
    foreach ($result['node'] as $nid => $data) {
      $photo_node = node_load($nid);
      $photos[] = $photo_node;
    }
  }

  $variables['photos'] = $photos;
}
