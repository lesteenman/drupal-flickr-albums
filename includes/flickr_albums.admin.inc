<?php

// TODO: Add a sync status page

function flickr_albums_settings_form() {
  $form = array();

  // TODO: Check if we have the correct permissions (read?), and give an error otherwise!

  $form['flickr_albums_userId'] = array(
    '#type' => 'textfield',
    '#title' => t('User ID'),
    '#description' => t('The user ID or email of your Flickr account.') . ' ' . l(t('Don\'t know your user ID?'), 'http://idgettr.com'),
    '#default_value' => variable_get('flickr_albums_userId', '11467783@N05'), // TODO: Clearly remove this userID before deploying.
  );

  // TODO: Implement this!
  $form['flickr_albums_token'] = array(
    '#type' => 'textfield',
    '#title' => t('Token for private pictures'),
    '#description' => t('Go to') . ' ' . l(t('http://www.flickr.com/services/api/keys/'), 'http://www.flickr.com/services/api/keys/', array('attributes' => array('target' => '_blank')))
        . ' ' . t('and edit your API key details, set \'authentication type\' to web application and as \'callback url\': <em>http://phpflickr.com/tools/auth/auth.php</em>') . '<br />' .
        t('Visit') . ' ' . l(t('http://phpflickr.com/tools/auth/'), 'http://phpflickr.com/tools/auth/', array('attributes' => array('target' => '_blank')))
        . '' . t('and fill in your API and Secret keys to generate a token, copy the token in this field.'),
    '#default_value' => variable_get('flickrgallery_token', NULL),
    '#states' => array(
      'visible' => array(
        ':input[name="flickrgallery_private_pictures"]' => array('value' => 1),
      ),
    ),
  );

  // TODO: Auth is broken: https://github.com/dan-coulter/phpflickr/issues/53

  $form['flickr_albums_batch_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Cron batch size'),
    '#description' => t('The amount of photos to fetch each cron run.'),
    '#default_value' => variable_get('flickr_albums_batch_size', 500),
  );

  $form['flickr_albums_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Page title'),
    '#description' => t('The title of the page with albums'),
    '#default_value' => variable_get('flickr_albums_title', t('Photos')),
  );

  $form['flickr_albums_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path for this module\'s page'),
    '#description' => t('Set the path for this module\'s page, where all albums can be found. Defaults to /photos. Leave empty if you do not want to use this page, but create your own view with Flickr Albums instead.'),
    '#default_value' => variable_get('flickr_albums_path', 'flickr-photos'),
  );

  // TODO: Add an option to select the ordering of albums on the default page.
  // TODO: Add an option to select the ordering of photos in an album.

  // TODO: Add:
  // - photos per page
  // - albums per page

  $form['#validate'][] = 'flickr_albums_settings_form_validate';
  $form['#submit'][] = 'flickr_albums_settings_form_submit';

  return system_settings_form($form);
}

function flickr_albums_settings_form_submit($form, &$form_state) {
  // Check if path is provided, if so rebuild the menu
  if ($form_state['values']['flickr_albums_path'] != $form['flickr_albums_path']['#default_value']) {
    variable_set('flickr_albums_path', $form_state['values']['flickr_albums_path']);
    if (menu_rebuild()) {
      drupal_set_message(t('Menu has been rebuilt'));
    }
  }

  // TODO: Update album info when changing any important settings (token, userId, )
}

function flickr_albums_status_form() {
  $form = array();

  $form['flickr_albums_sync_status'] = array(
    '#markup' => flickr_albums_sync_status(),
  );

  $form['flickr_albums_get_batch_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update 300 photos/videos immediately'),
    '#description' => t('Update all albums immediately. This might take a long time if you have a lot of albums and/or photos in your Flickr account.'),
    '#submit' => array('flickr_albums_update_batch'),
  );

  return system_settings_form($form);
}

function flickr_albums_sync_status() {
  $albums_total = variable_get('flickr_albums_albums_total', 0);
  $photos_total = variable_get('flickr_albums_photos_total', 0);
  $videos_total = variable_get('flickr_albums_videos_total', 0);
  $albums_synced = variable_get('flickr_albums_albums_synced', 0);
  $photos_synced = variable_get('flickr_albums_photos_synced', 0);
  $videos_synced = variable_get('flickr_albums_videos_synced', 0);

  $last_update = variable_get('flickr_albums_last_sync', 'never');
  $cron_running = !!variable_get('flickr_albums_cron_running', 0);

  // Format last update date
  $statusText = "";
  if ($last_update !== 'never') {
    $statusText = "Synced $photos_synced/$photos_total photos, $videos_synced/$videos_total videos and $albums_synced/$albums_total albums.";
    $last_update = date('Y-m-d H:i:s', $last_update);
  }

  $warning = '';
  if ($cron_running) {
    $warning = 'WARNING: The last manual or cron run did not finish! This might be an indicator of too large a batch size for your server.';
  }

  return <<<EOD
<div style='border: 1px solid #ddd; margin-bottom: 15px; padding: 5px;'>
  <h3>Status</h3>
  <b>$warning</b>
  $statusText<br>
  Last updated: $last_update
</div>
EOD;
}

function flickr_albums_update_batch() {
  // TODO: Use the drupal batch api
  $userId = variable_get('flickr_albums_userId', null);
  if (!$userId) {
    drupal_set_message('No userID set', 'error');
    return;
  }

  variable_set('flickr_albums_cron_running', 1);

  module_load_include('inc', 'flickr_albums', 'includes/flickr_albums.cron');
  flickr_albums_process_batch(300);

  variable_set('flickr_albums_cron_running', 0);
}

function flickr_albums_settings_form_validate($form, &$form_state) {
  module_load_include('module', 'flickrapi');
  if ($nsid = flickrapi_get_user_nsid($form_state['values']['flickr_albums_userId'])) {
    $form_state['values']['flickr_albums_userId'] = $nsid;
  }
  else {
    form_set_error('flickr_albums_userId', t('Not a valid Flickr UserID.'));
  }

  if (!intval($form_state['values']['flickr_albums_batch_size'])) {
    form_set_error('flickr_albums_batch_size', t('Not a valid batch size'));
  }
}
