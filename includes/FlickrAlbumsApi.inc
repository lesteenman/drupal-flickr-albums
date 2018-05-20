<?php

/**
 * Service to get photos and albums from the database.
 */
class FlickrAlbumsApi {

  /**
   * Returns all flickr_albums albums from the database.
   *
   * @return array
   *   An array of EntityMetadataWrappers containing all albums,
   *   or an empty array if none were found.
   */
  public static function getAlbums() {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', FLICKR_ALBUMS_ALBUM_NODE_TYPE);
    $result = $query->execute();

    if (!empty($result['node'])) {
      $node = $result['node'];

      $albums = [];
      foreach (array_keys($node) as $node_id) {
        $album_wrapper = entity_metadata_wrapper('node', $node_id);
        $albums[] = $album_wrapper;
      }

      return $albums;
    }

    return [];
  }

  /**
   * Returns a specific flickr_albums album from the database.
   *
   * @param string $album_id
   *   The Flickr id of the desired album.
   *
   * @return EntityMetadataWrapper|null
   *   An EntityMetadataWrapper containing the requested album, or
   *   NULL if the album was not found.
   */
  public static function getAlbum($album_id) {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', FLICKR_ALBUMS_ALBUM_NODE_TYPE)
      ->fieldCondition('flickr_albums_flickr_id', 'value', $album_id);
    $result = $query->execute();

    $logger = FlickrAlbumsServiceContainer::service('logging');
    $logger->log("Getting album $album_id, result = " . substr(print_r($result, true), 0, 1000));

    if (empty($result['node'])) {
      $logger->log("Returning NULL");
      return NULL;
    }

    $node_id = array_keys($result['node'])[0];
    $album_wrapper = entity_metadata_wrapper('node', $node_id);

    return $album_wrapper;
  }

  /**
   * Returns a specific flickr_albums photo from the database.
   *
   * @param string $album_id
   *   The Flickr id of the album the desired photo is in.
   * @param string $photo_id
   *   The Flickr id of the desired photo.
   *
   * @return EntityMetadataWrapper|null
   *   An EntityMetadataWrapper containing the requested photo, or
   *   NULL if the photo was not found.
   */
  public static function getPhoto($album_id, $photo_id) {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', FLICKR_ALBUMS_MEDIA_NODE_TYPE)
      ->fieldCondition('flickr_albums_album_id', 'value', $album_id)
      ->fieldCondition('flickr_albums_flickr_id', 'value', $photo_id);
    $result = $query->execute();

    if (!empty($result['node'])) {
      $node_id = array_keys($result['node'])[0];
      return NULL;
    }
    $photo_node = node_load($node_id);
    return entity_metadata_wrapper('node', $photo_node);
  }

  /**
   * Get various sized URLs for a flickr_albums photo.
   *
   * @param EntityMetadataWrapper $photo
   *   The photo for which you want the URLs.
   *
   * @return array
   *   An associative array of image URLs at various sizes.
   */
  public static function photoUrls(EntityMetadataWrapper $photo) {
    $id = $photo->flickr_albums_flickr_id->value();
    $farm = $photo->flickr_albums_farm->value();
    $server = $photo->flickr_albums_server->value();
    $secret = $photo->flickr_albums_secret->value();
    $original_secret = $photo->flickr_albums_original_secret->value();
    $original_format = $photo->flickr_albums_original_format->value();

    // Resized Format:  https://farm{farm-id}.staticflickr.com/{server-id}/{id}_{secret}_[mstzb].jpg
    // Original Format: https://farm{farm-id}.staticflickr.com/{server-id}/{id}_{o-secret}_o.(jpg|gif|png)
    $base = "https://farm{$farm}.staticflickr.com/$server/{$id}_{$secret}_";

    $sizes = [
    // 75x75.
      'small_square' => $base . "s.jpg",
    // 150x150.
      'large_square' => $base . "q.jpg",
    // 100 longest side.
      'thumbnail' => $base . "t.jpg",
    // 320 longest side.
      'small' => $base . "n.jpg",
      'original' => "https://farm{$farm}.staticflickr.com/$server/{$id}_{$original_secret}_o.$original_format",
    ];

    // Larger medium and large photos are only available for photos after
    // march 1st 2012.
    if ($photo->flickr_albums_date_upload->value() < strtotime('march 1 2012')) {
      // 640 longest side.
      $sizes['medium'] = $base . "z.jpg";
      // 1024 longest side.
      $sizes['large'] = $base . "b.jpg";
    }
    else {
      // 800 longest side.
      $sizes['medium'] = $base . "c.jpg";
      // 1600 longest side.
      $sizes['large'] = $base . "h.jpg";
    }

    return $sizes;
  }

}
