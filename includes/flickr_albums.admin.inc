<?php

function flickr_albums_settings_form() {
  $form = array();

  $form['flickr_albums_userId'] = array(
    '#type' => 'textfield',
    '#title' => t('User ID'),
    '#description' => t('The user ID or email of your Flickr account.') . ' ' . l(t('Don\'t know your user ID?'), 'http://idgettr.com'),
    '#default_value' => variable_get('flickr_albums_userId', null),
  );

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
    '#description' => t('Set the path for this module\'s page, where all albums can be found. Defaults to /photos'),
    '#default_value' => variable_get('flickr_albums_path', 'flickr-photos'),
  );

  $form['flickr_albums_block_newest_count'] = array(
    '#type' => 'textfield',
    '#title' => t('Albums in \'newest albums\' block'),
    '#description' => t('The amount of albums to show in the \'newest albums\' block'),
    '#default_value' => variable_get('flickr_albums_block_newest_count', 3),
  );

  $form['flickr_albums_block_top_count'] = array(
    '#type' => 'textfield',
    '#title' => t('Albums in \'top albums\' block'),
    '#description' => t('The amount of albums to show in the \'top albums\' block'),
    '#default_value' => variable_get('flickr_albums_block_top_count', 3),
  );

  $form['#validate'][] = 'flickr_albums_settings_form_validate';
  $form['#submit'][] = 'flickr_albums_settings_form_submit';

  return system_settings_form($form);
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

  $newest_count = $form_state['values']['flickr_albums_block_newest_count'];
  if (!intval($newest_count) || intval($newest_count) < 0) {
    form_set_error('flickr_albums_block_newest_count', t('Not a valid count'));
  }

  $top_count = intval($form_state['values']['flickr_albums_block_top_count']);
  if (!intval($top_count) || intval($top_count) < 0) {
    form_set_error('flickr_albums_block_top_count', t('Not a valid count'));
  }
}

function flickr_albums_settings_form_submit($form, &$form_state) {
  // Check if path is provided, if so rebuild the menu
  if ($form_state['values']['flickr_albums_path'] != $form['flickr_albums_path']['#default_value']) {
    variable_set('flickr_albums_path', $form_state['values']['flickr_albums_path']);
    if (menu_rebuild()) {
      drupal_set_message(t('Menu has been rebuild'));
    }
  }
}
