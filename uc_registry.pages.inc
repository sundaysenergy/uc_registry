<?php
// $Id: uc_registry.pages.inc,v 1.1 2009/03/27 16:08:33 rszrama Exp $

/**
 * @file
 * Page callback and functions for registries.
 */


/**
 * Display a registry for viewing or altering.
 *
 * @param $rid
 *   The ID of the registry to display.
 * @return
 *   HTML output for the registry display and form.
 */
function uc_registry_display($rid = NULL) {
  global $user;
  $output = '';
  drupal_add_js('sites/all/themes/mightynest/limitchars.js');
  
  // $own defines whether the registry is owned by the current user or not.
  // This affects how many parts of the page are rendered.
  $own = FALSE;

  // Default to the current user's registry if no registry ID is specified.
  if (empty($rid)) {
    $rid = uc_registry_get_rid();
    $own = TRUE;
  }
  elseif ($rid == uc_registry_get_rid()) {
    $own = TRUE;
  }
  
  // Attempt to load the registry.
  $registry = uc_registry_load($rid);
  
  // Handle a non-existent registry.
  if (!$registry) {
    // If the requested list was for the current user...
    if ($own) {
      // Display a message letting them know their list is empty.
      drupal_set_title(t('Registry'));

      return '<p><a href="/shop" class="start">Start building your Registry &#187;</a></p>';
    }
    else {
      // Otherwise send them to the search form.
      drupal_set_message(t('The registry you requested could not be found.  Perhaps you can try looking for it through the registry search form below.'));
      drupal_goto('registry/search');
    }
  }
  
  if ($registry->address->firstname) {
    $page_title = $registry->title;
  }

  drupal_set_title(check_plain($page_title));

  // Add the settings form if the user is viewing his own registry.
  if ($own) {
    if (!$user->uid) {
      drupal_set_message(t('You must <a href="!login_url">login</a> or <a href="!register_url">register</a> to save your registry.', array('!login_url' => url('user/login'), '!register_url' => url('user/register'))));
    }

    $collapsed = TRUE;

    if (empty($registry->address->firstname) || empty($registry->address->lastname) || empty($registry->address->addr1) || empty($registry->address->city) || empty($registry->address->postcode)) {
      $collapsed = FALSE;
    }

    $output .= drupal_get_form('uc_registry_settings_form', $registry, $collapsed);
  }

  $items = uc_registry_get_contents($rid);
  
  if ($registry->address->description) {
    $registry_info = '<div class="registry-description">' . filter_xss($registry->address->description) . '<span class="name">-' . filter_xss($registry->address->firstname) . ' ' . $registry->address->lastname ;
    if ($registry->address->co_registrant) {
      $registry_info .= ' and ' . filter_xss($registry->address->co_registrant);
    }    
    $registry_info .= '</span></div>';
  }
  
  if ($user->uid && !is_numeric(arg(1))) {
    $addthis_url = 'addthis:url="http://mightynest.com/registry/' . $rid . '"';
  }
  
  $registry_info .= '<div class="wlist-share"><div class="addthis_toolbox"><div class="custom_images"><a ' . $addthis_url . ' class="addthis_button_email"><img src="/sites/all/themes/mightynest/images/share-email.gif" width="25" height="25" alt="Email" /></a><a ' . $addthis_url . ' class="addthis_button_facebook"><img src="/sites/all/themes/mightynest/images/share-facebook.gif" width="26" height="25" alt="Facebook" /></a><a ' . $addthis_url . ' class="addthis_button_twitter"><img src="/sites/all/themes/mightynest/images/share-twitter.gif" width="26" height="25" alt="Twitter" /></a></div></div><script type="text/javascript" src="http://s7.addthis.com/js/250/addthis_ridget.js#username=xa-4b4e48877468911f"></script><script type="text/javascript">var addthis_share = {"url":\'' . $addthis_url . '\',"title":"' . $registry->title . '", "email_template": \'wishlist_share\'};</script><!-- AddThis Button END -->Share this registry!</div>';
  
  $registry_info .= '<span class="names">' . filter_xss($registry->address->firstname) . ' ' . filter_xss($registry->address->lastname) . '</span>';
  if ($registry->address->co_registrant) {
    $registry_info .= '<br /><span class="names">' . filter_xss($registry->address->co_registrant) . '</span>';
  }
  if ($registry->address->maiden_name) {
    $registry_info .= '<br />Maiden name: ' . filter_xss($registry->address->maiden_name);
  }
  if ($registry->address->city) {
    $registry_info .= '<br />' . filter_xss($registry->address->city) . ', ' . uc_zone_get_by_id($registry->address->zone);
  }
  $event_info = array();
  if ($registry->address->event_type) {
    $event_info[] = filter_xss($registry->address->event_type);
  }
  if ($registry->address->firstname && $registry->expiration != 0) {
    $event_info[] = date('n/d/Y',$registry->expiration+21600);
  }
  if ($registry->address->firstname && ($registry->expiration || $registry->address->event_type)) {
    $registry_info .= '<br /><br /><span class="event-date">Expected Arrival Date: ' . implode(', ',$event_info) . '</span>';
  }
  if ($registry->address->baby_type) {
    $registry_info .= '<br /><span class="event-date">It\'s a ' . $registry->address->baby_type . '!</span>';
  }

  if (empty($items)) {
    if ($own) {
      $output .= '<p><a href="/shop" class="start">Start building your Registry &#187;</a></p>';
    }
    else {
      $output .= '<p><i>No products have been added to this Registry.</i></p>';
    }
  }
  else {
    $output .= '<div id="uc-wishlist-view-form">' . drupal_get_form('uc_registry_view_form', $items, $rid, $own) . '</div>';
  }
  if (!$user->uid) {
    $registry_info .= '<br /><br />Is this your registry? <a href="/user/login?destination=registry">Click here.</a>';
  }

  return '<span class="registry-info">' . $registry_info . '</span>' . $output;
}

// Build the user registry settings form.
function uc_registry_settings_form($form_state, $registry, $collapsed = FALSE) {  
  // Attempt to load the wish list.
  // If there's no first name, this is the first time the user has filled out the form. Do not pre-fill expiration.
  if (!$registry->address->firstname) {
    $expiration = '';
  }
  elseif ($registry->expiration) {
    $expiration = date_convert($registry->expiration,DATE_UNIX,DATE_DATETIME);
  }
  global $user;
  drupal_add_js(drupal_get_path('module', 'uc_registry') .'/uc_registry.js');
  $form = array();
  if (is_numeric($_GET['nid'])) {
    $form['product_nid'] = array(
      '#type' => 'hidden',
      '#value' => $_GET['nid'],
    );
    if (is_numeric($_GET['qty'])) {
      $form['product_qty'] = array(
        '#type' => 'hidden',
        '#value' => $_GET['qty'],
      );
    }
    if ($_GET['data']) {
      $form['product_data'] = array(
        '#type' => 'hidden',
        '#value' => filter_xss($_GET['data'])
      );
    }
  }
  
  if (arg(1) == 'create') {
    $submit_value = "Create my Registry";
    $form['create_new'] = array(
      '#type' => 'hidden',
      '#value' => 'create_new',
    );
    $title = check_plain($user->name) . '\'s Registry';
  }
  else {
    $submit_value = "Save";
    $title = $registry->title;
  }

  $form['registry'] = array(
    '#type' => 'fieldset',
    '#title' => t('Edit your registry information'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
  );
  $form['registry']['rid'] = array(
    '#type' => 'hidden',
    '#value' => $registry->rid,
  );
  $form['registry']['title'] = array(
    '#type' => 'hidden',
    '#title' => t('Title'),
    '#default_value' => $title,
    '#required' => TRUE,
  );
  $form['registry']['expiration'] = array(
    '#type' => 'date',
    '#title' => t('Event or expiration date'),
    '#default_value' => $expiration,
    '#description' => t('If this registry is associated with an event or will no longer be relevant on a specific date, enter it here.'),
  );
  
  if ($user->uid) {
    $addresses = uc_select_address($user->uid, 'delivery', 'apply_address(\'delivery\', this.value);', t('Saved addresses'), TRUE);
    if (!empty($addresses)) {
      $form['registry']['delivery_address_select'] = $addresses;
      unset($form['registry']['delivery_address_select']['#suffix']);
    }
  }

  if (uc_address_field_enabled('first_name')) {
    $form['registry']['delivery_first_name'] = uc_textfield(uc_get_field_name('first_name'), $registry->address->firstname, uc_address_field_required('first_name'));
  }
  if (uc_address_field_enabled('last_name')) {
    $form['registry']['delivery_last_name'] = uc_textfield(uc_get_field_name('last_name'), $registry->address->lastname, uc_address_field_required('last_name'));
  }
  $form['registry']['maiden_name'] = array(
    '#type'   => 'textfield',
    '#title'  => t('Maiden Name'),
    '#default_value' => $registry->address->maiden_name,
    '#size' => 32,
  );
  $form['registry']['co_registrant'] = array(
    '#type'   => 'textfield',
    '#title'  => t("Co-registrant's Full Name"),
    '#default_value' => $registry->address->co_registrant,
    '#size' => 32,
  );
  if (uc_address_field_enabled('street1')) {
    $form['registry']['delivery_street1'] = uc_textfield(uc_get_field_name('street1'), $registry->address->addr1, uc_address_field_required('street1'), NULL, 64);
  }
  if (uc_address_field_enabled('street2')) {
    $form['registry']['delivery_street2'] = uc_textfield(uc_get_field_name('street2'), $registry->address->addr2, uc_address_field_required('street2'), NULL, 64);
  }
  if (uc_address_field_enabled('city')) {
    $form['registry']['delivery_city'] = uc_textfield(uc_get_field_name('city'), $registry->address->city, uc_address_field_required('city'));
  }
  if (uc_address_field_enabled('country')) {
    $form['registry']['delivery_country'] = uc_country_select(uc_get_field_name('country'), $registry->address->country, NULL, 'name', uc_address_field_required('country'));
  }
  if (uc_address_field_enabled('zone')) {
    if (isset($_POST['delivery_country'])) {
      $country_id = intval($_POST['delivery_country']);
    }
    else {
      $country_id = $registry->address->country;
    }
    $form['registry']['delivery_zone'] = uc_zone_select(uc_get_field_name('zone'), $registry->address->zone, NULL, $country_id, 'name', uc_address_field_required('zone'));
    if (isset($_POST['panes']) && count($form['registry']['address']['delivery_zone']['#options']) == 1) {
      $form['registry']['address']['delivery_zone']['#required'] = FALSE;
    }
  }
  if (uc_address_field_enabled('postal_code')) {
    $form['registry']['delivery_postal_code'] = uc_textfield(uc_get_field_name('postal_code'), $registry->address->postcode, uc_address_field_required('postal_code'), NULL, 10, 10);
  }
  if (uc_address_field_enabled('phone')) {
    $form['registry']['delivery_phone'] = uc_textfield(uc_get_field_name('phone'), $registry->address->phone, uc_address_field_required('phone'), NULL, 32, 16);
  }
  unset($form['registry']['expiration']);
  $form['registry']['event'] = array(
    '#type' => 'fieldset',
    '#collabsible' => FALSE,
    '#title' => 'Is this for an event?',
  );    
  $form['registry']['event']['event_type'] = array(
    '#type'   => 'select',
    '#title'  => t('Event type'),
    '#default_value' => $registry->address->event_type,
    '#options' => array(
        t('Baby')
      ),
    '#required'   => TRUE,
    '#weight' => 7,
  );
  $form['registry']['event']['expiration'] = array(
    '#type'   => 'date_select',
    '#title'  => t('Expected Arrival Date'),
    '#date_format' => 'm-d-Y',
    '#date_year_range' => '0:+20',
    '#default_value' => $expiration,
    '#required'   => FALSE,
    '#weight' => 8,
  );
  $form['registry']['event']['baby_type'] = array(
    '#type'   => 'select',
    '#title'  => t("Baby's Gender"),
    '#options' => array(
      'surprise'  => 'Surprise',
      'girl'  => 'Girl',
      'boy'   => 'Boy',
    ),
    '#default_value' => $registry->address->baby_type,
    '#weight' => 9,
  );
  $form['registry']['event']['description'] = array(
    '#type' => 'textarea',
    '#title' => 'Personal message',
    '#default_value' => $registry->address->description,
    '#weight' => 10,
    '#resizable' => FALSE,
    '#cols' => 35,
    '#rows' => 3,
    '#suffix' => '<div id="wltext-limit">You have 250 characters left.</div>',
  );

  $form['registry']['save'] = array(
    '#type' => 'submit',
    '#value' => $submit_value,
    '#weight' => 10,
  );

  return $form;
}

function uc_registry_settings_form_validate($form, &$form_state) {
  global $user;
  
  if ($form_state['values']['create_new'] != 'create_new') {
    $registry = uc_registry_load($rid);
    if (!$registry) {
      drupal_set_message(t('Could not find the specified registry.'), 'error');
      return FALSE;
    }
    if ($registry->uid != $user->uid && !user_perm('administer store')) {
      drupal_set_message(t('You do not have permission to edit this registry.'), 'error');
      return FALSE;
    }
  }
}

function uc_registry_settings_form_submit($form, &$form_state) {
  
  if ($form_state['values']['create_new'] == 'create_new') {
    $rid = uc_registry_create_registry();
    if (!$rid) {
      drupal_set_message(t('Could not create registry. Adding item failed.'), 'error');
      return FALSE;
    }
  }
  $expiration = date_convert($form_state['values']['expiration'],DATE_DATETIME,DATE_UNIX);

  $address = array(
    'firstname' => $form_state['values']['delivery_first_name'],
    'lastname' => $form_state['values']['delivery_last_name'],
    'co_registrant' => $form_state['values']['co_registrant'],
    'maiden_name' => $form_state['values']['maiden_name'],
    'addr1' => $form_state['values']['delivery_street1'],
    'addr2' => $form_state['values']['delivery_street2'],
    'city' => $form_state['values']['delivery_city'],
    'country' => $form_state['values']['delivery_country'],
    'zone' => $form_state['values']['delivery_zone'],
    'postcode' => $form_state['values']['delivery_postal_code'],
    'phone' => $form_state['values']['delivery_phone'],
    'description' => $form_state['values']['description'],
    'event_type' => $form_state['values']['event_type'],
    'baby_type' => $form_state['values']['baby_type']
  );
  
  $registry_name = $form_state['values']['delivery_first_name'] . ' ' . $form_state['values']['delivery_last_name'];
  if ($form_state['values']['co_registrant'] != '') {
    $registry_name .= ' and ' . $form_state['values']['co_registrant'];
  }
  $new_title = $registry_name . '\'s Registry';
  
  if ($form_state['values']['create_new'] == 'create_new') {
    $rid = uc_registry_get_rid();
  }
  else {
    $rid = $form_state['values']['rid'];
  }
  
  if ($form_state['values']['product_nid']) {
    parse_str($form_state['values']['product_data'],$data);
    print_r($form_state['values']);
    uc_registry_add_item($form_state['values']['product_nid'], $form_state['values']['product_qty'], $data, $rid);
  }

  uc_registry_update_registry($rid, $new_title, $expiration, (object) $address);

  if ($form_state['values']['create_new'] == 'create_new') {
    drupal_set_message(t('Your registry has been created. <a href="/shop">Start shopping now >></a>'));
  }
  else {
    drupal_set_message(t('Your registry has been updated.'));
  }
  drupal_goto('registry');
}

/**
 * Display a page allowing the customer to view his/her registry.
 */
function uc_registry_view_form($form_state, $items, $rid, $own) {
  $form = array();
    
      $sets = array();
      foreach ($items as $index => $row) {
        $grouping = '';
        //if (isset($this->view->field[$grouping_field])) {
          $grouping = $row->tid;
          //if ($this->view->field[$grouping_field]->options['label']) {
            $grouping = $grouping;
          //}
        //}
        $sets[$grouping][$index] = $row;
      }

  
  $form['sets'] = array(
    //'#tree' => TRUE,
  );

  foreach ($sets as $index => $reg_items) {
    // Load each registry product and add it to the form array.
    // $index = our term id
    $form['sets'][$index]['#tree'] = TRUE;
    $form['sets'][$index]['items'] = array(
      '#tree' => TRUE,
    );
    foreach ($reg_items as $item) {
      $node = node_load($item->nid);
      $element = array();
  
      $element['nid'] = array('#type' => 'value', '#value' => $node->nid);
      $element['rpid'] = array('#type' => 'value', '#value' => $item->rpid);
      $element['module'] = array('#type' => 'value', '#value' => 'uc_product');
  
      if ($own) {
        $element['remove'] = array('#type' => 'checkbox');
      }
  
      $item->haveqty = 0;
      if (is_array($item->purchase)) {
        $item->haveqty = count($item->purchase);
      }
      
      if ($node->field_manufacturer[0]['nid']) {
        $brand = node_load($node->field_manufacturer[0]['nid']);
        $brand_title = ' by ' . $brand->title;
      }
  
      $element['title'] = array(
        '#value' => '<span class="title"><b>' . filter_xss($node->title) . '</b>' . filter_xss($brand_title) . '</span>',
      );
  
      $description = '';

			// Add any attribute descriptions:
			$description .= uc_product_get_description($item);
			
      $description .=	' <div><a href="#TB_inline?height=300&width=400&inlineId=full-description-' . $node->nid . '" title="' .  $node->title . '" class="thickbox">Product description &#187;</a></div><div id="full-description-' . $node->nid . '" style="display:none">' . check_markup($node->body,2,FALSE) . '</div>';
      
      if ($description) {
        $element['description'] = array('#value' => $description);
      }
  
      $element['#total'] = $item->price;
  
      $element['data'] = array('#type' => 'hidden', '#value' => serialize($item->data));
  
      $element['wantqty'] = array(
        '#type' => 'textfield',
        //'#value' => $item->qty,   // Necessary with Disabled
        '#default_value' => $item->qty,
        '#size' => 5,
        '#maxlength' => 6,
        '#disabled' => $own ? FALSE : TRUE
      );
      if (!$own) {
        $element['wantqty']['#value'] = $item->qty;
      }
      $element['haveqty'] = array(
        '#type' => 'textfield',
        '#value' => $item->haveqty,   // Necessary with Disabled
        '#default_value' => $item->haveqty,
        '#size' => 5,
        '#maxlength' => 6,
        '#disabled' => TRUE
      );
      $element['qty'] = array(
        '#type' => 'textfield',
        '#size' => 5,
        '#maxlength' => 6,
        '#default_value' => '',
      );
      if (!variable_get('uc_registry_show_combined_addcart_button', null)) {
        $element['addcart'] = array(
          '#type' => 'submit',
          '#name' => 'addcart-'. $item->rpid,
          '#value' => t('Add to cart'),
        );
      } 
  
      $form['sets'][$index]['items'][] = $element;
    }
  } 
  
  $form['buttons_top'] = array(
    '#type' => 'fieldset',
    '#attributes' => array('class' => 'buttons'),
    '#weight' => -99,
  );
  
  if ($own) {
    $form['buttons_top']['continue'] = array(
      '#type' => 'markup',
      '#value' => '<a class="wlist-continue" href="/shop">Add more products to your registry</a>',
    );
  }
  
  if (variable_get('uc_registry_show_combined_addcart_button', null)) {
    $form['buttons_top']['addcart_top'] = array(
      '#type' => 'submit',
      '#value' => t('Add Items to Cart'),
    );
  }
  
  $form['buttons'] = array(
    '#type' => 'fieldset',
    '#attributes' => array('class' => 'buttons'),
  );
  
  if ($own) {
    $form['buttons']['continue'] = array(
      '#type' => 'markup',
      '#value' => '<a class="wlist-continue" href="/shop">Add more products to your registry</a>',
    );
  }
  
  if (variable_get('uc_registry_show_combined_addcart_button', null)) {
    $form['buttons']['addcart'] = array(
      '#type' => 'submit',
      '#value' => t('Add Items to Cart'),
    );
  }

  if ($own) {
    $form['own'] = array(
      '#type' => 'value',
      '#value' => TRUE,
    );
    $form['buttons']['update'] = array(
      '#type' => 'submit',
      '#value' => t('Update Registry'),
    );
    $form['buttons_top']['update_top'] = array(
      '#type' => 'submit',
      '#value' => t('Update Registry'),
    );
  }

  $form['rid'] = array(
    '#type' => 'hidden',
    '#value' => $rid,
  );
  //print_r($form);
  return $form;
}

function theme_uc_registry_view_form($form) {
  //print_r($form);
  
  $show_combined_addcart_button = FALSE;
  if (variable_get('uc_registry_show_combined_addcart_button', null)) {
    $show_combined_addcart_button = TRUE;
  }
  
  $form_top = drupal_render($form['buttons_top']);
  
  $own = isset($form['own']) ? TRUE : FALSE;

  if ($show_combined_addcart_button) {
    $header = array(array('data' => 'Products <span class="wlist-total">(Total number of products in registry: ' . count($form['#parameters'][2]) . ')</span>','class' => 'products'), '', array('data' => t('Price'), 'align' => 'right', 'class' => 'total'), array('data' => t('Wants'), 'class' => 'wanted'), array('data' => t('Has'), 'class' => 'have'), array('data' => t('Purchase'), 'class' => 'qty'));
  }
  else {
    $header = array(array('data' => t('Products'),'class' => 'products'), array('data' => t('Price'), 'align' => 'right', 'class' => 'total'), array('data' => t('Wants'), 'class' => 'wanted'), array('data' => t('Has'), 'class' => 'have'), array('data' => t('Purchase'), 'class' => 'qty'), array('data' => t(''), 'class' => 'purchase'));
  }

  if ($own) {
    array_unshift($header, array('data' => t('Remove'), 'class' => 'remove'));
    $attributes = 'registry-own';
  }
  else {
    $attributes = 'registry-other';
  }
  // we want to change this to foreach(sets), foreach $form['set']['items'], blah
  foreach ($form['sets'] as $index => $set) {
    // hack, because i can't get element_children to work like i'd expect
    if (is_numeric($index)) {
      $rows = array();
      foreach (element_children($set['items']) as $i) {
        $nid = $set['items'][$i]['nid']['#value'];
        $product = node_load($nid);
        $thumb = theme('imagecache','product_list',$product->field_image_cache[0]['filepath'], $product->title, '');
        $preview = imagecache_create_path('product_preview',$product->field_image_cache[0]['filepath']);
        
        $desc = drupal_render($form['sets'][$index]['items'][$i]['title']);
        $desc .= drupal_render($form['sets'][$index]['items'][$i]['description']);
        $row = array(
          //uc_product_get_picture($nid, 'cart'),
          array('data' => l($thumb,$preview,array('html' => TRUE,'attributes' => array('class' => 'thickbox'))), 'class' => 'image'),
          array('data' => $desc, 'class' => 'description'),
          array('data' => '<span class="price">' . uc_currency_format($form['sets'][$index]['items'][$i]['#total']) . '</span>', 'nowrap' => 'nowrap', 'class' => 'total'),
          $set['items'][$i]['wantqty'] ? array('data' => drupal_render($form['sets'][$index]['items'][$i]['wantqty']), 'class' => 'wanted') : '',
          $set['items'][$i]['haveqty'] ? array('data' => drupal_render($form['sets'][$index]['items'][$i]['haveqty']), 'class' => 'have') : '',
          $set['items'][$i]['qty'] ? array('data' => drupal_render($form['sets'][$index]['items'][$i]['qty']), 'class' => 'qty') : '',
        );
        if (!$show_combined_addcart_button) {
          array_push(&$row, $set['items'][$i]['addcart'] ? drupal_render($form['sets'][$index]['items'][$i]['addcart']) : '');
        }
    
        if (isset($set['items'][$i]['remove'])) {
          array_unshift($row, array('data' => drupal_render($form['sets'][$index]['items'][$i]['remove']), 'class' => 'remove'));
        }
    
        $rows[] = array('data' => $row, 'valign' => 'top');
      }
      $term = taxonomy_get_term($index);
      // we want, basically, foreach $sets, theme(table) with a caption of the taxo name
      $output .= '<div class="registry-form-products">'. theme('table', NULL, $rows,'',$term->name) .'</div>';
    }
    
  }

  return '<div class="' . $attributes . '">' . $form_top . theme('table',$header,NULL) . $output . drupal_render($form) . '</div>';
}

function uc_registry_view_form_validate($form, &$form_state) {
  //print_r($form_state['values']);
  // Find registry item associated with 'add to cart' submitted.
  foreach ($form_state['values'] as $key => $val) {
    if (strpos($key, 'addcart-') === 0) {
      $rpid = intval(substr($key, 8));
    }
  }
  // If adding item to cart, check quantities.
  if ($rpid) {
    foreach ($form_state['values'] as $key => $item) {
      /* In case the customer clicks the 'add to cart' for
       * this item after setting the qty to 0, they should
       * not get an error, since they probably mean to purchase
       * this item. So set qty=1 if qty==0
       */
      if (is_array($item)) {
        foreach ($item['items'] as $pkey => $product) {
          if ($item['items'][$pkey]['rpid'] == $rpid) {
            if ($item['items'][$pkey]['qty'] === "0" || $item['items'][$pkey]['qty'] == "") {
              $form_state['values'][$key]['items'][$pkey]['qty'] = 1;
            }
            elseif (!is_numeric($item['items'][$pkey]['qty'])) {
              form_set_error($key . '][items][' . $pkey . '][qty', t('You have entered an invalid quantity. '));
            }
          }
        }
      }
    }
  }
}

function uc_registry_view_form_submit($form, &$form_state) {
  if (isset($form_state['values']['op']) && !empty($form_state['values']['op'])) {
    if ($form_state['values']['op'] == $form_state['values']['update']) {
      //print_r((object) $form_state['values']);
      uc_registry_update_item_object((object) $form_state['values']);

      drupal_set_message(t('Your registry has been updated.'));

      $form_state['redirect'] = 'registry/'. $form_state['values']['rid'];
    }
    // Add all items to cart
    if ($form_state['values']['op'] == $form_state['values']['addcart']) {
      uc_registry_add_items_to_cart((object) $form_state['values']);
      drupal_set_message(t('The item(s) have been added to your shopping cart.'));
      //$form_state['redirect'] = 'cart';
    }
  }
  else {
    // Find registry item associated with 'add to cart' submitted.
    foreach ($form_state['values'] as $key => $val) {
      if (strpos($key, 'addcart-') === 0) {
        $rpid = intval(substr($key, 8));
      }
    }

    if ($rpid) {
      foreach ($form_state['values'] as $key => $item) {
        /*
         * uc_registry_add_items_to_cart() does not have a way
         * for us to define which specific item we want to purchase.
         * It takes in the whole form. So we have to unset the
         * quantities on those elements that we don't want to purchase!
         */
        if (is_array($item)) {
          foreach ($item['items'] as $pkey => $product) {
            if ($item['items'][$pkey]['rpid'] == $rpid) {
              if ($item['items'][$pkey]['qty'] === "0" || $item['items'][$pkey]['qty'] == "") {
                $form_state['values'][$key]['items'][$pkey]['qty'] = 1;
              }
            }
            else {
              $form_state['values'][$key]['items'][$pkey]['qty'] = 0;
            }
          }
        }
      }

      uc_registry_add_items_to_cart((object) $form_state['values']);
    }
  }
}

// Display the registry search form.
function uc_registry_search_form($form_state, $keywords = '') {
  global $user;
  $form = array();
  
  $first_name = check_plain($_GET['first_name']);
  $last_name = check_plain($_GET['last_name']);
  $city = check_plain($_GET['city']);
  $country = check_plain($_GET['country']);
  $zone = check_plain($_GET['zone']);
  
  
  
  if (arg(1) == 'search') {
    $query_args = array();
    $where = array();
    
    if (!empty($first_name)) {
      $where[] = "r.address LIKE '%%%s%%'";
      $query_args[] = $first_name;
    }
    
    if (!empty($last_name)) {
      $where[] = "r.address LIKE '%%%s%%'";
      $query_args[] = $last_name;
    }
    
    if (!empty($city)) {
      $where[] = "r.address LIKE '%%city%%%s%%'";
      $query_args[] = $city;
    }
    
    if (!empty($country)) {
      $where[] = "r.address LIKE '%%country%%%s%%'";
      $query_args[] = $country;
    }
    
    if (!empty($zone)) {
      $where[] = "r.address LIKE '%%zone%%%s%%'";
      $query_args[] = $zone;
    }
    
    $where = implode(' AND ',$where);
  
    if (!empty($first_name) || !empty($last_name) || !empty($city) || !empty($country) || !empty($zone)) {
      // Check for user, registry title, or address matches.
      $result = db_query("SELECT DISTINCT r.rid, r.title, r.address, r.expiration FROM {uc_registries} AS r JOIN {users} AS u ON r.uid = u.uid WHERE " . $where . " ORDER BY r.title", $query_args);
      $search_executed = TRUE;
    }
    else {
      //$result = pager_query("SELECT wid, title FROM {uc_registries} ORDER BY title", 25);
    }
    if (arg(1) == 'search') { 
      $form_prefix = '<div class="search-again">Didn\'t find what you were looking for? Search again:</div>';
    }
    
    $rows = array();
    $i=1;
    while ($registry = db_fetch_object($result)) {
      if ($registry->expiration != 0) {
        $res_expiration = date('n/d/Y',$registry->expiration);
      }
      $address = unserialize($registry->address);
      $rows[] = array(
        array('data' => $i,'class' => 'number'),
        array('data' => l(filter_xss($address->lastname, array()) . ', ' . filter_xss($address->firstname, array()),'registry/' . $registry->rid),'class' => 'name'),
        array('data' => filter_xss($address->co_registrant),'class' => 'add_name'),
        array('data' => filter_xss($address->city, array()) . ', ' . uc_zone_get_by_id($address->zone),'class' => 'location'),
        array('data' => $res_expiration,'class' => 'date'),
      );
      $i++;
    }
  
    if (!empty($rows)) {
      $result_count = count($rows) . t(' Results:');
      $output = theme('table',array('#','Name','Co-Registrant','Location','Event date'),$rows);
    }
    else {
      $output = t('<div class="result-count">No registries found.</div>');
    }
  

    $form['output'] = array(
      '#value' => '<div class="nests"> </div><div><div class="result-count">' . $result_count .'</div>'. $output .'</div>',
    );
  }

  $form['search'] = array(
    '#type' => 'fieldset',
    '#prefix' => $form_prefix,
  );
  $form['search']['first_name'] = array(
    '#type' => 'textfield',
    '#title' => t('First Name'),
    '#default_value' => $first_name,
    '#size' => 20,
  );
  $form['search']['last_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Last Name'),
    '#default_value' => $last_name,
    '#size' => 20,
  );
  $form['search']['city'] = array(
    '#type' => 'textfield',
    '#title' => t('City'),
    '#default_value' => $city,
    '#size' => 20,
  );
  if (uc_address_field_enabled('country')) {
    $form['search']['delivery_country'] = uc_country_select(uc_get_field_name('country'), '', NULL, 'name', FALSE);
  }
  if (uc_address_field_enabled('zone')) {
    if (isset($_POST['delivery_country'])) {
      $country_id = intval($_POST['delivery_country']);
    }
    else {
      $country_id = $registry->address->country;
    }
    $form['search']['delivery_zone'] = uc_zone_select(uc_get_field_name('zone'), $_POST['delivery_zone'], NULL, $country_id, 'name', FALSE);
    if (isset($_POST['panes']) && count($form['search']['delivery_zone']['#options']) == 1) {
      $form['search']['delivery_zone']['#required'] = FALSE;
    }
  }
  $form['search']['delivery_country']['#options'] = array('' => t('Please Select')) + $form['search']['delivery_country']['#options'];
  if (!$_GET['country']) {
    $form['search']['delivery_country']['#default_value'] = '';
  }
  $form['search']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Find registry'),
	  '#submit' => array('uc_registry_search_form_submit'),
  );
  
  return $form;
}

function uc_registry_search_form_submit($form, &$form_state) {
  if (empty($form_state['values']['first_name']) && empty($form_state['values']['last_name']) && empty($form_state['values']['city']) && empty($form_state['values']['delivery_country']) && empty($form_state['values']['delivery_zone'])) {
    drupal_goto('registry/search');
  }
  else {
    if ($form_state['values']['first_name']) {
      $query .= '&first_name=' . $form_state['values']['first_name'];
    }
    if ($form_state['values']['last_name']) {
      $query .= '&last_name=' . $form_state['values']['last_name'];
    }
    if ($form_state['values']['city']) {
      $query .= '&city=' . $form_state['values']['city'];
    }
    if ($form_state['values']['delivery_country']) {
      $query .= '&country=' . $form_state['values']['delivery_country'];
    }
    if ($form_state['values']['delivery_zone']) {
      $query .= '&zone=' . $form_state['values']['delivery_zone'];
    }
    drupal_goto('registry/search',$query);
  }
}

/**
 * Display a registry for viewing or altering.
 *
 * @param $rid
 *   The ID of the registry to display.
 * @return
 *   HTML output for the registry display and form.
 */
function uc_registry_create($nid = NULL, $qty = NULL, $data = NULL) {
  global $user;
  if (!$user->uid) {
    drupal_set_message('You must log in, or create an account, in order to create a registry.');
    drupal_goto('user/login','destination=registry/create');
    return;
  }
  $output = '';
  drupal_add_js('sites/all/themes/mightynest/limitchars.js');

  $rid = uc_registry_get_rid();
  
  if (!empty($rid)) {
    drupal_set_message('A registry already exists for your account.');
    drupal_goto('registry');
    return;
  }

  $output .= drupal_get_form('uc_registry_settings_form', NULL, FALSE);

  return $output;
}
