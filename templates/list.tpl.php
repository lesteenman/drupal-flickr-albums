<?php

/**
 * @file
 * Default theme implementation to show a list of all albums for the Flickr Albums module.
 *
 * @ingroup themeable
 */
?>

List of albums

<?php
foreach ($albums as $album) {
  $view = node_view($album, 'teaser');
  print drupal_render($view);
}
?>
