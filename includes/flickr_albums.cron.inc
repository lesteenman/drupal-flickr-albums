<?php

module_load_include('module', 'flickrapi');
module_load_include('inc', 'flickr_albums', 'includes/flickr_albums.api');

function flickr_albums_process_batch($count) {
  $flickr = flickrapi_phpFlickr();
  $token = variable_get('flickr_albums_token', null);

  // Check if we have the correct permissions (read?)

  // At any point, once we reach $count photos: return. The next iteration will automatically continue.
  // Get all photosets
  // For all new photosets:
  // - Create photoset
  // - Import up to $count photos, in batches of 500 (Flickr max)
  // For all existing photosets:
  // - Check if the 'updated' date is later than our last node update
  //
  // For all photos we import:
  // - Save all relevant fields
}
