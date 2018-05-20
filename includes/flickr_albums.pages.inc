<?php

/**
 * @file
 * Contains hooks relevant for most of the flickr_albums pages.
 *
 * Hooks for showing albums, album teasers, photos, videos, lists of
 * either of those media items, etc.
 */

module_load_include('inc', 'flickr_albums', 'includes/flickr_albums.api');

/**
 * Page callback: shows a list of albums(?)
 *
 * @return array
 *   A render array for a page containing a list of albums(?)
 */
function flickr_albums_list() {
  // TODO: Only show albums with photos that the current user
  // should be able to see!
  // TODO: Use an API service.
  $albums = [];
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', FLICKR_ALBUMS_ALBUM_NODE_TYPE)
    ->fieldOrderBy('flickr_albums_weight', 'value', 'asc');
  $result = $query->execute();

  if (!empty($result['node'])) {
    foreach ($result['node'] as $nid => $data) {
      $album_node = node_load($nid);
      $view = node_view($album_node, 'teaser');
      $albums[] = drupal_render($view);
    }
  }

  $build = [
    '#theme' => 'flickr_albums_list',
    '#description' => t('List of albums'),
    '#albums' => $albums,
  ];

  return drupal_render($build);
}

/**
 * Callback that's used by flickr_albums_preprocess_node.
 *
 * @see flickr_albums_preprocess_node()
 */
function flickr_albums_preprocess_photo(&$variables, $hook) {
  $photo_node = $variables['node'];
  $photo_wrapper = entity_metadata_wrapper('node', $photo_node);

  $variables['urls'] = flickr_albums_photo_urls($photo_wrapper);
}

/**
 * Callback that's used by flickr_albums_preprocess_node.
 *
 * @see flickr_albums_preprocess_node()
 */
function flickr_albums_preprocess_album_teaser(&$variables, $hook) {
  $album_node = $variables['node'];
  $album_wrapper = entity_metadata_wrapper('node', $album_node);

  $primary = $album_wrapper->flickr_albums_primary_photo->value();
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', FLICKR_ALBUMS_MEDIA_NODE_TYPE)
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

/**
 * Callback that's used by flickr_albums_preprocess_node.
 *
 * @see flickr_albums_preprocess_node()
 */
function flickr_albums_preprocess_album(&$variables, $hook) {
  $album_node = $variables['node'];
  $album_wrapper = entity_metadata_wrapper('node', $album_node);

  $photos = array();
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', FLICKR_ALBUMS_MEDIA_NODE_TYPE)
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
