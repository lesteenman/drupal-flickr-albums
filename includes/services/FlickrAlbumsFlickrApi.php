<?php

module_load_include('module', 'flickrapi');

/**
 * Thin wrapper for the PHP flickr api.
 */
class FlickrAlbumsFlickrApi {
  public $flickrApi;

  /**
   * Initializes the FlickrAlbumsFlickrApi object.
   */
  public function __construct() {
    $this->flickrApi = flickrapi_phpFlickr();
  }

  /**
   * Passes any calls on to the actual PHP Flickr api.
   */
  public function __call($name, $args) {
    return call_user_func_array(array($this->flickrApi, $name), $args);
  }

  /**
   *
   */
  public function test($message = true) {
    if (!$this->flickrApi) {
      $this->flickrApi = flickrapi_phpFlickr(TRUE);
    }

    $error = false;

    $library = libraries_load('phpFlickr');
    if (empty($library) || $library['loaded'] === FALSE) {
      $error = t("It seems the phpFlickr library has not yet been installed.");
    }
    else if (!$this->flickrApi) {
      $error = t("It seems like the Flickr API has not been set up properly. Please !configure the Flickr API properly.", ['!configure' => l(t('configure'), 'admin/config/media/flickrapi')]);

      $api_key = variable_get('flickrapi_api_key', '');
      $api_secret = variable_get('flickrapi_api_secret', '');
      $phpFlickr = null; // new phpFlickr($api_key, $api_secret);
    }

    if ($error && $message) {
      drupal_set_message($error, 'error');

      $variable_service = FlickrAlbumsServiceContainer::service('variables');
      $last_watchdog_warning = $variable_service->get('last_watchdog_api_warning', 0);
      if (time() - $last_watchdog_warning > 600) {
        $variable_service->set('last_watchdog_api_warning', time());
        $logger = FlickrAlbumsServiceContainer::service('logger');
        $logger->warn($error);
      }
    }

    return !$error;
  }

}
