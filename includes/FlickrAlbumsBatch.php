<?php

module_load_include('php', 'flickr_albums', 'includes/FlickrAlbumsServiceContainer');

/**
 * Service to perform various batch operations for flickr_albums.
 *
 * Contains functions that are used using the batch API to split
 * up larger tasks like syncing.
 */
class FlickrAlbumsBatch {

  /**
   * Initialize the synchronization of a batch of media items.
   *
   * Should be called before self::batchSet, whether it's called manually
   * or by something like batch_set.
   *
   * @param string|int $sync_total
   *   Either an integer indicating how many photos should be synced
   *   in total, or 'all' to sync all remaining photos.
   * @param int $batch_size
   *   The number of photos that should be synchronized with each batch.
   * @param array $context
   *   (optional) A batch API context array.
   */
  public static function syncInit($sync_total, int $batch_size, array &$context = NULL) {
    $variable_service = FlickrAlbumsServiceContainer::service('variables');
    $logger = FlickrAlbumsServiceContainer::service('logger');
    $logger->log("Initialize");

    $variable_service->set('sync_running', 1);

    $logger->log('Initialize batch update');

    // Make sure we're up-to-date on the number of albums and photos.
    self::updateAllAlbums();

    $logger->log("Album count: " . count(FlickrAlbumsApi::getAlbums()));

    $photos_total = $variable_service->get('photos_total', 0);
    $videos_total = $variable_service->get('videos_total', 0);
    $photos_synced = $variable_service->get('photos_synced', 0);
    $videos_synced = $variable_service->get('videos_synced', 0);

    $photos_remaining = $photos_total - $photos_synced;
    $videos_remaining = $videos_total - $videos_synced;
    $total_remaining = $photos_remaining + $videos_remaining;

    $logger->log("We got ordered to sync $sync_total photos in batches of $batch_size");

    if ($sync_total === 'all') {
      $sync_total = $total_remaining;
    }
    else {
      $sync_total = min($total_remaining, $sync_total);
    }

    if ($sync_total < 0) {
      // TODO: Likely unneccessary.
      // Something likely went wrong earlier, sync a sane amount of photos
      // to try and correct this.
      $sync_total = 10;
    }

    $batch_size = min($batch_size, $sync_total);

    $batch_count = intval(ceil($sync_total / $batch_size));

    $logger->log("Actually update $sync_total photos in batch sizes of $batch_size, so a total of $batch_count batches.");
    if ($context) {
      $context['results'] = [
        'batch_size' => $batch_size,
        'batch_count' => $batch_count,
      ];

      $context['progress_message'] = t("Updating individual albums...");
      $context['message'] = self::progressMessage(0, $batch_count);
    }
  }

  /**
   * Synchronizes a single batch of media items.
   *
   * @param int|array $context
   *   If an int, this is the batch size. Can be used when manually
   *   synchronizing a batch of photos.
   *   If an array, it should be a batch API context array.
   */
  public static function syncBatch(&$context) {
    $variable_service = FlickrAlbumsServiceContainer::service('variables');
    $flickr = FlickrAlbumsServiceContainer::service('flickr_api');
    $logger = FlickrAlbumsServiceContainer::service('logger');

    if (is_int($context)) {
      $batch_size = $context;
      $context = NULL;
    }
    else {
      if (is_array($context['results']) && isset($context['results']['batch_size'])) {
        $batch_size = $context['results']['batch_size'];
        $context['sandbox']['batch_size'] = $batch_size;

        $context['sandbox']['progress'] = 0;
        $context['sandbox']['max'] = $context['results']['batch_count'];

        $logger->log("Syncing the first batch! Results = " . print_r($context['results'], TRUE));
        $logger->log("Sandbox = " . print_r($context['sandbox'], TRUE));
      }
      else {
        $batch_size = $context['sandbox']['batch_size'];
      }
    }

    // TODO: Give the last album and photo where we were in $context['sandbox'].
    $token = $variable_service->get('token', NULL);

    // TODO: Check if we have the correct permissions (read?)
    $userId = $variable_service->get('userId', NULL);
    if (!$userId) {
      $logger->log('cron could not fetch Flickr albums, as no userId has been given.');
      return;
    }

    $logger->log("process a batch of size $batch_size for user " . $userId);

    // Total photos found.
    $photos_total = $variable_service->get('photos_total');
    // Total photos found.
    $videos_total = $variable_service->get('videos_total');

    // Photos synced total, including previous batches.
    $photos_synced = 0;
    // Photos synced total, including previous batches.
    $videos_synced = 0;

    // All photos/videos that have been processed this batch.
    $media_done = 0;
    // Videos that have been processed this batch.
    $videos_done = 0;
    // Photos that have been processed this batch.
    $photos_done = 0;

    $albums = FlickrAlbumsApi::getAlbums();

    $last_album = $last_media = NULL;
    $find_last_album = $find_last_media = NULL;

    $batch_start = TRUE;
    $batch_kill = FALSE;
    if ($context && isset($context['sandbox']['last_album'])) {
      // $batch_kill = TRUE;

      $find_last_album = $context['sandbox']['last_album'];
      $find_last_media = $context['sandbox']['last_media'];

      $batch_start = FALSE;
    }

    // Update all albums.
    foreach ($albums as $album_wrapper) {
      $album_id = $album_wrapper->flickr_albums_flickr_id->value();
      if (!$album_id) {
        $values = [];
        foreach ($album_wrapper->getPropertyInfo() as $key => $val) {
          $values[$key] = $album_wrapper->$key->value();
        }
        $logger->error("Found an album without a flickr id! " . substr(print_r($values, TRUE), 0, 15000));
        $batch_kill = TRUE;
        break;
      }

      $album_photos_total = intval($album_wrapper->flickr_albums_count_photos->value());
      $album_videos_total = intval($album_wrapper->flickr_albums_count_videos->value());

      // Skip until we reach the last album we stopped at.
      if ($find_last_album !== NULL) {
        if ($find_last_album !== $album_id) {
          continue;
        }

        $logger->log("Found the last album again, pick it up from here. album_id=$album_id");
        $find_last_album = NULL;
      }

      // If we're already over our batch size, don't actually
      // process this album.
      if ($media_done >= $batch_size - 1) {
        break;
      }

      $last_album = $album_id;

      // Get the amount of photos we've synced in this album before.
      // TODO: Move to some class
      $photos_synced_query = new EntityFieldQuery();
      $photos_synced_query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', FLICKR_ALBUMS_MEDIA_NODE_TYPE)
        ->fieldCondition('flickr_albums_album_id', 'value', $album_id);
      $photos_synced_query->fieldCondition('flickr_albums_media', 'value', 'photo');
      $album_photos_synced = intval($photos_synced_query->count()->execute());

      // Get the amount of videos we've synced in this album before.
      // TODO: Move to some class
      $videos_synced_query = new EntityFieldQuery();
      $videos_synced_query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', FLICKR_ALBUMS_MEDIA_NODE_TYPE)
        ->fieldCondition('flickr_albums_album_id', 'value', $album_id);
      $videos_synced_query->fieldCondition('flickr_albums_media', 'value', 'video');
      $album_videos_synced = intval($videos_synced_query->count()->execute());

      if ($album_photos_synced > $album_photos_total || $album_videos_synced > $album_videos_total) {
        $logger->warn("Album {$album_id} has $album_photos_synced/$album_photos_total photos, $album_videos_synced/$album_videos_total videos!");
      }

      // Last time we've fully synced this album to Drupal.
      $last_local_update = $album_wrapper->flickr_albums_last_local_update->value();

      // Last update on Flickr.
      $date_update = $album_wrapper->flickr_albums_date_update->value();
      if ($last_local_update >= $date_update) {
        $logger->log("Don't actually update album $album_id, we already have all $album_photos_total ($album_photos_synced) photos and $album_videos_total ($album_videos_synced) videos. local update $last_local_update >= flickr update $date_update");
        $videos_synced += $album_videos_synced;
        $photos_synced += $album_photos_synced;
        continue;
      }

      $extras = 'url_o,original_format,last_update,date_taken,date_upload,media';
      $photos = $flickr->photosets_getPhotos($album_id, $extras)['photoset']['photo'];
      if (!is_array($photos)) {
        $logger->log("Invalid set of photos in album $album_id: " . substr(print_r($photos, TRUE), 0, 4000));
        return;
      }

      $photo_weight = 1;
      $all_album_photos_synced = TRUE;
      $album_media_done = 0;
      foreach ($photos as $photo) {
        // Skip until we reach the last photo we stopped at.
        if ($find_last_media !== NULL && $find_last_media !== $photo['id']) {
          // Done last batch.
          $album_media_done++;
          $photo_weight++;
          continue;
        }
        $find_last_media = NULL;

        $last_media = $photo['id'];

        // If this photo would go over the amount of photos we should sync in
        // this batch, stop. Check this at the start of the next loop instead
        // of the end of the last loop, as we don't want to set
        // all_albums_synced to false in that case.
        if ($media_done >= $batch_size) {
          $all_album_photos_synced = FALSE;
          break;
        }

        // Update the photo.
        list($photo_wrapper, $done) = self::updatePhoto($album_id, $photo, $photo_weight++);
        $album_media_done++;

        if ($done) {
          // $logger->log("Media done! {$photo_wrapper->flickr_albums_flickr_id->value()}");
          $media_done++;
        }

        // $logger->log("Wrapper is of type {$photo_wrapper->flickr_albums_media->value()}");
        if ($photo_wrapper->flickr_albums_media->value() === 'video') {
          $videos_synced++;
          if ($done) {
            $album_videos_synced++;
            $videos_done++;
          }
        }
        else {
          $photos_synced++;
          if ($done) {
            $album_photos_synced++;
            $photos_done++;
          }
        }
      }

      if ($all_album_photos_synced) {
        $logger->log("Completely synced album $album_id");

        if ($album_photos_total !== $album_photos_synced || $album_videos_total !== $album_videos_synced) {
          $logger->error("Attempt to mark album $album_id as done, while we only have " . json_encode($album_photos_synced) . " of " . json_encode($album_photos_total) . " photos synced, and " . json_encode($album_videos_synced) . " of " . json_encode($album_videos_total) . "! Media done was " . json_encode($album_media_done));
        }
        else {
          $album_wrapper->flickr_albums_last_local_update->set(time());
          $album_wrapper->save();
        }
      }
    }

    $variable_service->set('photos_synced', $photos_synced);
    $variable_service->set('videos_synced', $videos_synced);

    $variable_service->set('last_sync', time());

    if ($batch_kill) {
      // TODO: remove.
      return;
    }

    if ($context) {
      $context['message'] = self::progressMessage($context['sandbox']['progress'], $context['sandbox']['max']);

      if (!is_int($context['results'])) {
        $context['results'] = 0;
      }
      $context['results'] += $photos_done + $videos_done;

      $context['sandbox']['progress']++;

      if ($context['sandbox']['progress'] === $context['sandbox']['max']) {
        $logger->log("Finished! Sandbox = " . print_r($context['sandbox'], TRUE));
        $context['finished'] = 1;
      }
      else {
        $context['finished'] = 0;
        $context['sandbox']['last_album'] = $last_album;
        $context['sandbox']['last_media'] = $last_media;
        // TODO: Message?
      }
    }
  }

  /**
   * Finalize one or more batches of synchronization.
   *
   * Should be called after self::batchSet, whether it's called manually
   * or by something like batch_set.
   *
   * @param bool $success
   *   (optional) A boolean indicating whether the batch finished successfully.
   * @param mixed $results
   *   (optional) The results as set on the context in self::syncBatch().
   * @param array $operations
   *   (optional) The operations that have been executed.
   */
  public static function syncFinished($success = NULL, $results = NULL, array $operations = NULL) {
    $variable_service = FlickrAlbumsServiceContainer::service('variables');
    $logger = FlickrAlbumsServiceContainer::service('logger');

    $logger->log('Flickr albums batch finished!');
    $variable_service->set('sync_running', 0);

    if ($success) {
      drupal_set_message("Flickr Albums finished updating " . print_r($results, TRUE) . " photos and/or videos.");
    }
    elseif ($success !== NULL) {
      drupal_set_message("Error occured while doing update batches. Please refer to the recent system message log for more information.", 'error');
    }
  }

  /**
   * Perform a single batch of clearing synced media items.
   *
   * Should be used using batch_set.
   *
   * @param array $context
   *   A batch API context array.
   */
  public static function clearBatch(array &$context) {
    $variable_service = FlickrAlbumsServiceContainer::service('variables');
    $logger = FlickrAlbumsServiceContainer::service('logger');

    $context['finished'] = 0;
    // Continue from the last album and photo, or the first if not set.
    if (!$context['sandbox']['photos_done'] ?? FALSE) {
      $context['sandbox']['albums_done'] = 1;
      return;
    }

    if (!$context['sandbox']['videos_done'] ?? FALSE) {
      $context['sandbox']['albums_done'] = 1;
      return;
    }

    if (!$context['sandbox']['albums_done'] ?? FALSE) {
      $context['sandbox']['albums_done'] = 1;
      return;
    }

    // Loop over the albums and photos, and set update_times to 0.
    // Set the last album and photo in context.
    $context['finished'] = 1;
  }

  /**
   * Finish up a batch of clearing up synced media items.
   *
   * @param bool $success
   *   (optional) A boolean indicating whether the batch finished successfully.
   * @param mixed $results
   *   (optional) The results as set on the context in self::clearBatch().
   * @param array $operations
   *   (optional) The operations that have been executed.
   *
   * @see self::clearBatch
   */
  public static function clearFinished($success = NULL, $results = NULL, array $operations = NULL) {
    $variable_service = FlickrAlbumsServiceContainer::service('variables');
    $logger = FlickrAlbumsServiceContainer::service('logger');

    /* $variable_service->set('albums_synced', 0); */
    /* $variable_service->set('photos_synced', 0); */
    /* $variable_service->set('videos_synced', 0); */
    /* $variable_service->set('last_sync', 0); */
  }

  /**
   * Synchronizes basic info of all albums from Flickr to Drupal.
   */
  public static function updateAllAlbums() {
    $variable_service = FlickrAlbumsServiceContainer::service('variables');
    $logger = FlickrAlbumsServiceContainer::service('logger');
    $flickr_api = FlickrAlbumsServiceContainer::service('flickr_api');

    $token = $variable_service->get('token', NULL);

    $userId = $variable_service->get('userId', NULL);
    if (!$userId) {
      $logger->log('cron could not fetch Flickr albums, as no userId has been given.');
      return;
    }

    $flickr_albums = [];
    $result = NULL;
    while (!$result || $result['page'] < $result['pages']) {
      $page = $result ? $result['page'] + 1 : 1;
      $result = $flickr_api->photosets_getList($userId, $page);

      if ($result['pages'] === 0) {
        $logger->log('No albums found');
        return;
      }

      $logger->log("Existing albums = " . substr(print_r($flickr_albums, TRUE), 0, 5000) . ", \nadding " . substr(print_r($result, TRUE), 0, 5000));
      $flickr_albums += $result['photoset'];
    }

    $albums_total = 0;
    $photos_total = 0;
    $videos_total = 0;
    // Albums updated this run.
    $albums_updated = 0;

    foreach ($flickr_albums as $flickr_album) {
      $albums_total++;
      $photos_total += $flickr_album['photos'];
      $videos_total += $flickr_album['videos'];

      $done = self::updateAlbum($flickr_album, $albums_total);
      if ($done) {
        $albums_updated++;
      }
    }

    $logger->log("Updated album info, we have a total of $albums_total albums, of which $albums_updated were updated this run.");

    $variable_service->set('albums_total', $albums_total);
    $variable_service->set('photos_total', $photos_total);
    $variable_service->set('videos_total', $videos_total);
  }

  /**
   * Updates a specific album.
   *
   * @param array $flickr_album
   *   A Flickr album as returned by FlickrAlbumsFlickrApi.
   * @param int $order
   *   The order of this specific album. Albums with a lower order
   *   will be listed earlier in the list of albums. Albums with an
   *   equal order will have no guaranteed order.
   *
   * @return bool
   *   Whether or not the update happened. If it did not, it does
   *   not indicate an error, but simply that the album was
   *   up-to-date.
   */
  private static function updateAlbum(array $flickr_album, $order) {
    $logger = FlickrAlbumsServiceContainer::service('logger');
    $flickr_api = FlickrAlbumsServiceContainer::service('flickr_api');

    $flickr_id = $flickr_album['id'];
    if (!$flickr_id) {
      $logger->error("No flickr id found in album: " . substr(print_r($flickr_album, TRUE), 0, 1000));
    }
    $album_wrapper = FlickrAlbumsApi::getAlbum($flickr_id);

    if ($album_wrapper) {
      // Album exists, test if it has changed.
      if ($album_wrapper->flickr_albums_date_update->value() >= $flickr_album['date_update']) {
        if ($album_wrapper->flickr_albums_flickr_id->value()) {
          return FALSE;
        }
        else {
          $logger->log("Got an album that *should* be up-to-date, but doesn't have a flickr id! " . print_r([$album_wrapper->getPropertyInfo(), $album_wrapper->flickr_albums_flickr_id->value()], TRUE));
        }
      }
    }
    else {
      // Create a new empty album.
      $album_node = entity_create('node', [
        'type' => FLICKR_ALBUMS_ALBUM_NODE_TYPE,
      ]);
      $album_wrapper = entity_metadata_wrapper('node', $album_node);
    }

    $album_info = $flickr_api->photosets_getInfo($flickr_album['id']);

    // Make sure we're up to date on all changes to the album.
    $album_wrapper->title->set($album_info['title']['_content']);
    $album_wrapper->flickr_albums_description->set($album_info['description']['_content']);
    $album_wrapper->flickr_albums_flickr_id->set($flickr_id);
    $album_wrapper->flickr_albums_secret->set($flickr_album['secret']);
    $album_wrapper->flickr_albums_primary_photo->set($flickr_album['primary']);
    $album_wrapper->flickr_albums_server->set($flickr_album['server']);
    $album_wrapper->flickr_albums_farm->set($flickr_album['farm']);
    $album_wrapper->flickr_albums_count_photos->set($flickr_album['photos']);
    $album_wrapper->flickr_albums_count_videos->set($flickr_album['videos']);
    $album_wrapper->flickr_albums_date_update->set($flickr_album['date_update']);
    $album_wrapper->flickr_albums_date_create->set($flickr_album['date_create']);
    $album_wrapper->flickr_albums_weight->set($order);
    $album_wrapper->save();

    return TRUE;
  }

  /**
   * Updates a specific photo.
   *
   * @param string $flickr_album_id
   *   A Flickr id indicating what album the photo should be in.
   *   TODO: Check whether this is even necessary.
   * @param array $flickr_photo
   *   A Flickr photo as returned by FlickrAlbumsFlickrApi.
   *
   * @return array
   *   An array containing two items:
   *   First item: an EntityMetadataWrapper for the photo.
   *   Second item: a boolean indicating whether or not the
   *   update happened. If it did not, it does not indicate
   *   an error, but simply that the photo was up-to-date.
   */
  private static function updatePhoto($flickr_album_id, array $flickr_photo, int $weight) {
    $variable_service = FlickrAlbumsServiceContainer::service('variables');
    $logger = FlickrAlbumsServiceContainer::service('logger');

    $photo_wrapper = FlickrAlbumsApi::getPhoto($flickr_album_id, $flickr_photo['id']);

    if ($photo_wrapper) {
      // Already exist, check if it is still up-to-date.
      $logger->log("Skip photo? Wrapper = {$photo_wrapper->flickr_albums_date_update->value()}, album = {$photo_wrapper->flickr_albums_album_id}, flickr_photo = " . print_r($flickr_photo, TRUE));
      if ($flickr_photo['lastupdate'] <= $photo_wrapper->flickr_albums_date_update->value()) {
        return [$photo_wrapper, FALSE];
      }
    }
    else {
      // Create a new photo.
      $photo_node = entity_create('node', [
        'type' => FLICKR_ALBUMS_MEDIA_NODE_TYPE,
      ]);
      $photo_wrapper = entity_metadata_wrapper('node', $photo_node);
    }

    preg_match('/[0-9]+_([0-9a-zA-Z]+)_o.jpg/', $flickr_photo['url_o'], $matches);
    $original_secret = $matches[1];

    $photo_wrapper->title->set($flickr_photo['title']);
    $photo_wrapper->flickr_albums_flickr_id->set($flickr_photo['id']);
    $photo_wrapper->flickr_albums_secret->set($flickr_photo['secret']);
    $photo_wrapper->flickr_albums_original_secret->set($original_secret);
    $photo_wrapper->flickr_albums_server->set($flickr_photo['server']);
    $photo_wrapper->flickr_albums_farm->set($flickr_photo['farm']);
    $photo_wrapper->flickr_albums_original_format->set($flickr_photo['originalformat']);
    $photo_wrapper->flickr_albums_media->set($flickr_photo['media']);
    $photo_wrapper->flickr_albums_album_id->set($flickr_album_id);
    $photo_wrapper->flickr_albums_visibility_public->set($flickr_photo['ispublic'] ? 1 : 0);
    $photo_wrapper->flickr_albums_visibility_friends->set($flickr_photo['isfriend'] ? 1 : 0);
    $photo_wrapper->flickr_albums_visibility_family->set($flickr_photo['isfamily'] ? 1 : 0);
    $photo_wrapper->flickr_albums_date_update->set($flickr_photo['lastupdate']);
    $photo_wrapper->flickr_albums_date_taken->set(strtotime($flickr_photo['datetaken']));
    $photo_wrapper->flickr_albums_date_upload->set($flickr_photo['dateupload']);
    $photo_wrapper->flickr_albums_weight->set($weight);
    $photo_wrapper->save();

    return [$photo_wrapper, TRUE];
  }

  /**
   * Get a progress message for the batch API.
   *
   * @param int $progress
   *   The amount of batches that have been done so far.
   * @param int $max
   *   The maximum amount of batches.
   *
   * @return string
   *   A user-facing message indicating the progress.
   */
  private static function progressMessage($progress, $max) {
    return t("@progress of @max batches done.", ['@progress' => $progress, '@max' => $max]);
  }

}
