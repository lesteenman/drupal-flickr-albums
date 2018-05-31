<?php

/**
 * @file
 * Contains hooks relevant for the administration page.
 */

module_load_include('php', 'flickr_albums', 'includes/FlickrAlbumsServiceContainer');

/**
 * Page callback: Displays the form with settings for this module.
 *
 * @return array
 *   A render array for the settings page of the flickr_albums module.
 *
 * @see flickr_albums_menu()
 */
function flickr_albums_settings_form() {
  $variable_service = FlickrAlbumsServiceContainer::service('variables');
  $form = [];

  $flickrApi = FlickrAlbumsServiceContainer::service('flickr_api');
  $flickrApi->test();

  // TODO: Check if we have the correct permissions (read?), and give an error
  // otherwise!
  $form['flickr_albums_userId'] = [
    '#type' => 'textfield',
    '#title' => t('User ID'),
    '#description' => t("The user ID or email of your Flickr account.") . ' ' . l(t("Don't know your user ID?"), "http://idgettr.com"),
  // TODO: OBVIOUSLY remove this userID before deploying.
    '#default_value' => $variable_service->get('userId', '11467783@N05'),
  ];

  // TODO: Test this.
  $form['flickr_albums_token'] = [
    '#type' => 'textfield',
    '#title' => t('Token for private pictures'),
    '#description' => t('Go to') . ' ' . l(t('http://www.flickr.com/services/api/keys/'), 'http://www.flickr.com/services/api/keys/', ['attributes' => ['target' => '_blank']])
    . ' ' . t("and edit your API key details, set 'authentication type' to web application and as 'callback url': <em>http://phpflickr.com/tools/auth/auth.php</em>") . '<br />' .
    t('Visit') . ' ' . l(t('http://phpflickr.com/tools/auth/'), 'http://phpflickr.com/tools/auth/', ['attributes' => ['target' => '_blank']])
    . '' . t('and fill in your API and Secret keys to generate a token, copy the token in this field.'),
    '#default_value' => $variable_service->get('flickrgallery_token', NULL),
    '#states' => [
      'visible' => [
        ':input[name="flickrgallery_private_pictures"]' => ['value' => 1],
      ],
    ],
  ];

  $form['flickr_albums_cron_batch_size'] = [
    '#type' => 'textfield',
    '#title' => t('Cron batch size'),
    '#description' => t('The amount of photos to fetch each cron run.'),
    '#default_value' => $variable_service->get('cron_batch_size', 500),
  ];

  $form['flickr_albums_title'] = [
    '#type' => 'textfield',
    '#title' => t('Page title'),
    '#description' => t('The title of the page with albums'),
    '#default_value' => $variable_service->get('title', t('Photos')),
  ];

  $form['flickr_albums_path'] = [
    '#type' => 'textfield',
    '#title' => t("Path for this module's page"),
    '#description' => t("Set the path for this module's page, where all albums can be found. Defaults to /photos. Leave empty if you do not want to use this page, but create your own view with Flickr Albums instead."),
    '#default_value' => $variable_service->get('path', 'flickr-photos'),
  ];

  // TODO: Add an option to select the ordering of albums on the default page.
  // TODO: Add an option to select the ordering of photos in an album.
  // TODO: Add:
  // - photos per page
  // - albums per page.
  $form['#validate'][] = 'flickr_albums_settings_form_validate';
  $form['#submit'][] = 'flickr_albums_settings_form_submit';

  return system_settings_form($form);
}

/**
 * Validation function for flickr_albums_settings_form().
 */
function flickr_albums_settings_form_validate($form, &$form_state) {
  if ($nsid = flickrapi_get_user_nsid($form_state['values']['flickr_albums_userId'])) {
    $form_state['values']['flickr_albums_userId'] = $nsid;
  }
  else {
    form_set_error('flickr_albums_userId', t('Not a valid Flickr UserID.'));
  }

  if (!intval($form_state['values']['flickr_albums_cron_batch_size'])) {
    form_set_error('flickr_albums_cron_batch_size', t('Not a valid batch size'));
  }
}

/**
 * A callback for the submit function of flickr_albums_settings_form().
 */
function flickr_albums_settings_form_submit($form, &$form_state) {
  // Check if path is provided, if so rebuild the menu.
  if ($form_state['values']['flickr_albums_path'] != $form['flickr_albums_path']['#default_value']) {
    $variable_service = FlickrAlbumsServiceContainer::service('variables');
    $variable_service->set('path', $form_state['values']['flickr_albums_path']);
    if (menu_rebuild()) {
      drupal_set_message(t('Menu has been rebuilt'));
    }
  }

  // TODO: Also add a separate button for this on the sync status page.
  module_load_include('php', 'flickr_albums', 'includes/FlickrAlbumsBatch');
  FlickrAlbumsBatch::updateAllAlbums();
}

/**
 * Page callback: Displays the sync status for this module.
 *
 * The page displays the current sync status, like the number of
 * photos and albums still to be synchronized, and buttons to initiate
 * a synchronization.
 *
 * @return array
 *   A render array for the status page of the flickr_albums module.
 *
 * @see flickr_albums_menu()
 */
function flickr_albums_status_form() {
  $form = array();

  $form['flickr_albums_sync_status'] = [
    '#markup' => flickr_albums_sync_status(),
  ];

  $form['sync_total'] = [
    '#type' => 'textfield',
    '#default_value' => 'all',
    '#prefix' => t('Synchronize # items:'),
    '#suffix' => t("Giving a number will sync up to that number of photos that have not yet been synced. Entering 'all' will sync all photos, including previously synced.<br>"),
  ];

  $form['batch_size'] = [
    '#type' => 'textfield',
    '#default_value' => '200',
    '#prefix' => t('In batches of'),
  ];

  $form['submit'] = [
    '#type' => 'submit',
    '#value' => t('Start synchronization run'),
    '#disabled' => FALSE,
  ];

  $form['clear'] = array(
    '#type' => 'submit',
    '#value' => 'Reset all synchronized albums',
    '#prefix' => "<div>",
    '#suffix' => '<br>Do this only if something seems to have gone wrong, and you want to remove all albums that had been synchronized thus far.</div>',
    '#submit' => ['flickr_albums_clear_synced_items'],
    '#name' => 'clear',
    '#disabled' => FALSE,
    // '#button_type' => 'danger',
  );

  $form['#validate'][] = 'flickr_albums_status_form_validate';
  $form['#submit'][] = 'flickr_albums_run_batches';

  $flickrApi = FlickrAlbumsServiceContainer::service('flickr_api');
  if (!$flickrApi->test()) {
    $form['submit']['#disabled'] = TRUE;
    $form['clear']['#disabled'] = TRUE;
  }

  return $form;
}

/**
 * Validation function for flickr_albums_status_form().
 */
function flickr_albums_status_form_validate($form, &$form_state) {
  $sync_total = $form_state['values']['sync_total'];
  $batch_size = intval($form_state['values']['batch_size']);

  if ($sync_total !== 'all') {
    $sync_total = intval($sync_total);
    if (!is_int($sync_total) || $sync_total <= 0) {
      form_set_error('sync_total', t("Enter a valid number greater than 0 or 'all' to sync all photos and albums."));
    }
  }

  if (!is_int($batch_size)) {
    form_set_error('batch_size', t('Not a valid batch size.'));
  }
}

/**
 * Callback for the flickr_albums_status_form submit.
 *
 * Starts a batch run with the specified (in the form) number of
 * batches and batch size.
 */
function flickr_albums_run_batches($form, &$form_state) {
  $logger = FlickrAlbumsServiceContainer::service('logger');
  $variable_service = FlickrAlbumsServiceContainer::service('variables');

  $sync_total = $form_state['values']['sync_total'];
  $batch_size = $form_state['values']['batch_size'];

  $logger->log('Run batches: total=' . $sync_total . ', per batch=' . $batch_size);

  // Check if we have a Flickr userId.
  $user_id = $variable_service->get('userId', NULL);
  if (!$user_id) {
    drupal_set_message('No userID set', 'error');
    return;
  }

  /* $variable_service->set('sync_running', 1); */

  if (!$batch_size) {
    $logger->log("Got an invalid batch size in form state: " . print_r($form_state['values'], TRUE));
    return;
  }

  $batch = [
    'file' => drupal_get_path('module', 'flickr_albums') . '/includes/FlickrAlbumsBatch.php',
    'operations' => [
      ['FlickrAlbumsBatch::syncInit', [$sync_total, $batch_size]],
      ['FlickrAlbumsBatch::syncBatch', []],
    ],
    'finished' => 'FlickrAlbumsBatch::syncFinished',
    'title' => t('Getting album information'),
    'init_message' => t('Fetching general album information from Flickr'),
    // 'progress_message' => t('Fetched @current out of @total albums.'),.
    'error_message' => t('Synchronization failed'),
  ];

  $logger->log("Setting batch");

  batch_set($batch);

  // TODO: Is this bit necessary?
  /* $batch = &batch_get(); */
  /* $batch['progressive'] = FALSE; */

  /* $variable_service->set('sync_running', 0); */
}

/**
 * Callback function for the clear action of flickr_albums_status_form().
 */
function flickr_albums_clear_synced_items() {
  $logger = FlickrAlbumsServiceContainer::service('logger');
  $variable_service = FlickrAlbumsServiceContainer::service('variables');

  // TODO: Add confirmation!
  /* drupal_set_message('Would clear synced items now'); */
  /* return; */

  $logger->log("Clearing all synced items!");

  $sql = 'select nid from {node} n where n.type = :photo_type or n.type = :album_type';
  $result = db_query($sql, [
    ':photo_type' => FLICKR_ALBUMS_MEDIA_NODE_TYPE,
    ':album_type' => FLICKR_ALBUMS_ALBUM_NODE_TYPE,
  ]);
  $nodeIds = [];
  foreach ($result as $row) {
    $nodeIds[] = $row->nid;
  }
  node_delete_multiple($nodeIds);

  $variable_service->set('albums_total', 0);
  $variable_service->set('photos_total', 0);
  $variable_service->set('videos_total', 0);
  $variable_service->set('photos_synced', 0);
  $variable_service->set('photos_synced', 0);
  $variable_service->set('videos_synced', 0);

  // TODO: Do this in batches as well.
  /* $batch = [ */
  /*   'file' => drupal_get_path('module', 'flickr_albums') . '/includes/FlickrAlbumsBatch.php', */
  /*   'operations' => [ */
  /*     ['FlickrAlbumsBatch::clearBatch', []], */
  /*   ], */
  /*   'finished' => 'FlickrAlbumsBatch::clearFinished', */
  /*   'title' => t('Invalidating...'), */
  /*   'progress_message' => t('Invalidating all albums, photos and videos'), */
  /*   'error_message' => t('Failed to clear synced items.'), */
  /* ]; */

  /* $variable_service->set('albums_synced', 0); */
  /* $variable_service->set('photos_synced', 0); */
  /* $variable_service->set('videos_synced', 0); */
  /* $variable_service->set('last_sync', 0); */
}

/**
 * Returns HTML for flickr_albums_status_form.
 *
 * Returned HTML shows the current sync status of the module.
 *
 * @return string
 *   The status HTML.
 */
function flickr_albums_sync_status() {
  $variable_service = FlickrAlbumsServiceContainer::service('variables');
  $albums_total = $variable_service->get('albums_total', 0);
  $photos_total = $variable_service->get('photos_total', 0);
  $videos_total = $variable_service->get('videos_total', 0);
  $albums_synced = $variable_service->get('albums_synced', 0);
  $photos_synced = $variable_service->get('photos_synced', 0);
  $videos_synced = $variable_service->get('videos_synced', 0);

  $last_update = $variable_service->get('last_sync', 'never');
  $cron_running = !!$variable_service->get('sync_running', 0);

  // Format last update date.
  $last_update_text = $status_text = "";
  if ($last_update === 'never') {
    $status_text = t("No synchronization happened yet.");
  }
  else {
    $status_text = t("Synced @photos_synced of @photos_total photos, @videos_synced of @videos_total videos and @albums_synced of @albums_total albums.", [
      '@photos_synced' => $photos_synced,
      '@photos_total' => $photos_total,
      '@videos_synced' => $videos_synced,
      '@videos_total' => $videos_total,
      '@albums_synced' => $albums_synced,
      '@albums_total' => $albums_total,
    ]);
    $last_update_text = t('Last updated: @last_update', ['@last_update' => date('Y-m-d H:i:s', $last_update)]);
  }

  $warning = '';
  if ($cron_running) {
    $warning = t('WARNING: The last manual or cron run did not finish! This might be an indicator of too large a batch size for your server.<br>');
  }

  return <<<EOD
<div style='border: 1px solid #ddd; margin-bottom: 15px; padding: 0 5px 5px;'>
  <h3>Status</h3>
  <b>$warning</b>
  $status_text<br>
  $last_update_text
</div>
EOD;
}
