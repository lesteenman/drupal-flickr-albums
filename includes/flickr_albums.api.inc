<?php

function flickr_albums_api_get_album($albumId) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', ALBUM_NODE_TYPE)
    ->fieldCondition('flickr_albums_flickr_id', 'value', $albumId);
  $result = $query->execute();

  watchdog('flickr_albums', 'Result: ' . json_encode($result));

  if (!empty($result['node']))
    return entity_load('node', array_keys($result['node'])[0]);

  return null;
}

function flickr_albums_api_get_photo($albumId, $photoId) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', PHOTO_NODE_TYPE)
    ->fieldCondition('field_flickr_albums_album_id', 'value', $albumId)
    ->fieldCondition('field_flickr_albums_flickr_id', 'value', $photoId);
  $result = $query->execute();

  if (!empty($result['node']))
    return entity_load('node', array_keys($result['node'])[0]);

  return null;
}
