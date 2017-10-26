<?php

function flickr_albums_api_get_album($albumId) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', ALBUM_NODE_TYPE)
    ->fieldCondition('flickr_albums_flickr_id', 'value', $albumId);
  $result = $query->execute();

  watchdog('flickr_albums', 'Result: ' . json_encode($result));

  if (!empty($result['node'])) {
    $node = $result['node'];
    return node_load(array_keys($node)[0]);
  }

  return null;
}

function flickr_albums_api_get_photo($albumId, $photoId) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
    ->entityCondition('bundle', PHOTO_NODE_TYPE)
    ->fieldCondition('flickr_albums_album_id', 'value', $albumId)
    ->fieldCondition('flickr_albums_flickr_id', 'value', $photoId);
  $result = $query->execute();

  if (!empty($result['node']))
    return entity_load('node', array_keys($result['node'])[0]);

  return null;
}

function flickr_albums_photo_urls($photo) {
  // Resized Format:  https://farm{farm-id}.staticflickr.com/{server-id}/{id}_{secret}_[mstzb].jpg
  // Original Format: https://farm{farm-id}.staticflickr.com/{server-id}/{id}_{o-secret}_o.(jpg|gif|png)
 
  $id = $photo->flickr_albums_flickr_id->value();
  $farm = $photo->flickr_albums_farm->value();
  $server = $photo->flickr_albums_server->value();
  $secret = $photo->flickr_albums_secret->value();
  $original_secret = $photo->flickr_albums_original_secret->value();
  $original_format = $photo->flickr_albums_original_format->value();

  $base = "https://farm{$farm}.staticflickr.com/$server/{$id}_{$secret}_";

  $sizes = array(
    'small_square' => $base . "s.jpg", // 75x75
    'large_square' => $base . "q.jpg", // 150x150
    'thumbnail' => $base . "t.jpg", // 100 longest side
    'small' => $base . "n.jpg", // 320 longest side
    'original' => "https://farm{$farm}.staticflickr.com/$server/{$id}_{$original_secret}_o.$original_format",
  );

  // Larger medium and large photos are only available for photos after march 1st 2012.
  if ($photo->flickr_albums_date_upload->value() < strtotime('march 1 2012')) {
    $sizes['medium'] = $base . "z.jpg"; // 640 longest side
    $sizes['large'] = $base . "b.jpg"; // 1024 longest side
  }
  else {
    $sizes['medium'] = $base . "c.jpg"; // 800 longest side
    $sizes['large'] = $base . "h.jpg"; // 1600 longest side
  }

  return $sizes;
}
