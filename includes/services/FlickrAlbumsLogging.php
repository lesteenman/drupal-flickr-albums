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

  /**
   * Log a warning to Watchdog.
   *
   * @param string $log
   *   The string to log to Watchdog.
   */
  public function warn($log) {
    watchdog('flickr_albums', $log, [], WATCHDOG_WARNING);
  }

  /**
   * Log an error to Watchdog.
   *
   * @param string $log
   *   The string to log to Watchdog.
   */
  public function error($log) {
    watchdog('flickr_albums', $log, [], WATCHDOG_ERROR);
  }

}
