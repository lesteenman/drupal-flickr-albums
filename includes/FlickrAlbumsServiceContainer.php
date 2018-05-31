<?php

module_load_include('inc', 'flickr_albums', 'includes/FlickrAlbumsApi');

/**
 * Service container for services used by flickr_albums.
 *
 * Can be used to get various services, and can be made to
 * return mock versions of these services for tests.
 */
class FlickrAlbumsServiceContainer {
  private static $definitions = [
    'db_api' => [
      'file' => 'includes/FlickrAlbumsApi',
      'class' => 'FlickrAlbumsApi',
    ],

    'variables' => [
      'file' => 'includes/services/FlickrAlbumsVariables',
      'class' => 'FlickrAlbumsVariables',
    ],
    'logger' => [
      'file' => 'includes/services/FlickrAlbumsLogging',
      'class' => 'FlickrAlbumsLogging',
    ],
    'flickr_api' => [
      'file' => 'includes/services/FlickrAlbumsFlickrApi',
      'class' => 'FlickrAlbumsFlickrApi',
    ],
  ];

  private static $instances = [];

  /**
   * Get a service.
   *
   * @param string $service
   *   The desired service to get. Available services are defined in
   *   FlickrAlbumsServiceContainer::$definitions.
   *
   * @return object|null
   *   An instantiated version of the requested service class.
   */
  public static function service($service) {
    if (isset(self::$instances[$service])) {
      return self::$instances[$service];
    }

    if (!isset(self::$definitions[$service])) {
      watchdog('flickr_albums', "Unknown service $service.");
      return NULL;
    }

    // Load the include.
    $definition = self::$definitions[$service];
    module_load_include('php', 'flickr_albums', $definition['file']);

    // Create a new instance of the class, save it and return it.
    $class = $definition['class'];
    $instance = new $class();
    self::$instances[$service] = $instance;
    return $instance;
  }

  /**
   * Set the definition for a service.
   *
   * @param string $service
   *   The name of the service that's to be changed.
   * @param array $definition
   *   An array with the new service definition. Minimum required keys are
   *   'file' and 'class'.
   */
  public static function setDefinition($service, array $definition) {
    assert(isset($definition['file']));
    assert(isset($definition['class']));
    self::$definitions[$service] = $definition;

    // Throw away any old instances if we have them.
    unset(self::$instances[$service]);
  }

}
