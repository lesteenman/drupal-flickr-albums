<?php

/**
 * Service to get and set persistent variables.
 */
class FlickrAlbumsVariables {
  private static $prefix = 'flickr_albums_';

  /**
   * Get a persistent variable.
   *
   * @param string $variable
   *   The name of the variable to get.
   * @param mixed $default
   *   The default to return if the variable has not yet been set.
   *
   * @return mixed
   *   the previously set variable, or $default if it has not been set yet.
   */
  public function get($variable, $default = NULL) {
    return variable_get(self::$prefix . $variable, $default);
  }

  /**
   * Set a persistent variable.
   *
   * @param string $variable
   *   The name of the variable to set.
   * @param mixed $value
   *   The value to set.
   */
  public function set($variable, $value) {
    variable_set(self::$prefix . $variable, $value);
  }

  /**
   * Deletes a persistent variable.
   *
   * @param string $variable
   *   The name of the variable to delete.
   */
  public function del($variable) {
    variable_del(self::$prefix . $variable);
  }

}
