<?php

/**
 * Service to log messages to watchdog.
 */
class FlickrAlbumsLogging {

  /**
   * Log a message to Watchdog.
   *
   * @param string $log
   *   The string to log to Watchdog.
   */
  public function log($log) {
    watchdog('flickr_albums', $log);
  }

}
