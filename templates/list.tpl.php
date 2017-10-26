<?php

/**
 * @file
 * Default theme implementation to show a list of all albums for the Flickr Albums module.
 *
 * @ingroup themeable
 */
?>

<div class='flickr-albums-albums'>
  <?php
  foreach ($albums as $album) {
    print $album;
  }
  ?>
</div>
