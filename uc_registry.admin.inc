<?php
// $Id: uc_registry.admin.inc,v 1.1 2009/03/27 16:08:33 rszrama Exp $

/**
 * @file
 * Admin page callbacks and forms for registries.
 */


// Build the admin settings form.
function uc_registry_admin_settings_form() {
  $form = array();

  $form['uc_registry_default_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Default registry title'),
    '#default_value' => variable_get('uc_registry_default_title', "%user's registry"),
    '#description' => t("The default name of a new registry. The token %user will be replaced by the user's name."),
  );
  $form['uc_registry_allow_anonymous'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow anonymous users to create registries.'),
    '#default_value' => variable_get('uc_registry_allow_anonymous', FALSE),
    '#description' => t('Check this box to allow anonymous users to create registries. If not checked, anonymous visitors will be prompted to login or register before creating a registry.'),
  );
  $form['uc_registry_combined_addcart_group'] = array(
    '#type' => 'fieldset',
    '#title' => t('Combined Add Registry to Cart Button'),
    '#description' => t(''),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['uc_registry_combined_addcart_group']['uc_registry_show_combined_addcart_button'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use single button to add registry items to cart'),
    '#description' => t('If this option is enabled the registry display will have a single "Add To Cart" button for all items in the registry. If disabled every item in the registry will have its own "Add to Cart" button'),
    '#default_value' => variable_get('uc_registry_show_combined_addcart_button', FALSE),
  );
  $form['uc_registry_combined_addcart_group']['uc_registry_combined_addcart_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Combined Addcart Button Text'),
    '#default_value' => variable_get('uc_registry_combined_addcart_text', 'Add Registry Quantities to Cart'),
    '#description' => t('Button text for the combined AddCart button for registrys. This is ONLY relevant if the above option (Use single button to add registry items to cart) is enabled. Otherwise this setting does nothing!'),
  );

  // TODO: add uc_registry_default_length

  return system_settings_form($form);
}

// View and manage the registries on the site.
function uc_registry_admin() {
  $rows = array();

  $header = array(
    array('data' => t('User'), 'field' => 'u.name', 'sort' => 'asc'),
    array('data' => t('Title'), 'field' => 'r.title'),
    array('data' => t('Expiration date'), 'field' => 'r.expiration'),
    array('data' => t('Status'))
  );

  // Get a paged list of registries from the database.
  $result = pager_query("SELECT r.rid, r.uid, r.title, r.expiration, u.name FROM {uc_registries} AS r LEFT JOIN {users} AS u ON r.uid = u.uid". tablesort_sql($header), 25);

  while ($registry = db_fetch_object($result)) {
    // Build the operations array for the registry.
    $op = array(
      $registry->expiration < time() ? t('Expired') : t('Active'),
      l(t('Delete'), 'admin/store/customers/registry/'. $registry->rid .'/delete'),
    );

    $rows[] = array(
      $registry->name ? l(check_plain($registry->name), 'user/'. $registry->uid) : t('Anonymous'),
      l(filter_xss($registry->title), 'registry/'. $registry->rid),
      format_date($registry->expiration),
      implode(' | ', $op),
    );
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => t('No registries found.'), 'colspan' => 4));
  }

  return theme('table', $header, $rows) . theme('pager');
}

// Confirm the deletion of a registry.
function uc_registry_admin_delete_form($form_state, $registry) {
  $form = array();

  $form['registry'] = array(
    '#type' => 'value',
    '#value' => $registry,
  );

  return confirm_form($form, t('Are you sure you want to delete registry %title?', array('%title' => $registry->title)), 'admin/store/customers/registry', NULL, t('Delete'));
}

function uc_registry_admin_delete_form_submit($form, &$form_state) {
  // Delete the registry.
  uc_registry_delete($form_state['values']['registry']->rid);

  // Display a message and redirect back to the admin table.
  drupal_set_message(t('Registry %title deleted.', array('%title' => $form_state['values']['registry']->title)));

  $form_state['redirect'] = 'admin/store/customers/registry';
}

