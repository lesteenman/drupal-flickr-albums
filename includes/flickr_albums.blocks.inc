<?php

function flickr_albums_block_info() {
  $blocks = array(
    'newest_albums' => array(
      'info' => t('Newest Flickr albums'),
      'cache' => DRUPAL_CACHE_PER_ROLE,
    ),

    'top_albums' => array(
      'info' => t('Shows the first few albums, as ordered on your Flickr account'),
      'cache' => DRUPAL_CACHE_PER_ROLE,
    ),
  );

  return $blocks;
}

function flickr_albums_block_view() {
  switch ($delta) {
    case 'newest_albums':
      $albums = [];
      break;

    case 'top_albums':
      $albums = [];
      break;
  }

  return array(
    'content' => theme('album_list', array(
      'albums' => $albums,
    ),
  );
}
