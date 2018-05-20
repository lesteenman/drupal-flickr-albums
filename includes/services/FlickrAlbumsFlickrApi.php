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

}
