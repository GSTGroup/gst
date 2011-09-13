<?php
// $Id$ 
/** 
 * @file 
 * GST FormAPI Module
 * 
 */

// <editor-fold defaultstate="collapsed" desc="GROUP">

function gst_gstgroup_form_alter(&$form, &$form_state, $form_id) {
  
  // Fix the Group_Audience Widget so it isn't so lame
  // Existing Format:
  //  [My groups] =>
  //    [gid] => Name
  //  [Other groups] =>
  //    [gid] => Other group name
  //
  // -We will find all "Org" type groups and put them at the top, and put
  //    any Groups that are *below* the org below it in a hierarchy.
  
  $audOptions = $form['group_audience']['und']['#options'];
  $audMyGroups = (isset($audOptions['My groups'])) ? $audOptions['My groups'] : NULL;
  $audOthGroups = (isset($audOptions['Other groups'])) ? $audOptions['Other groups'] : NULL;
  $groups = array();
  if ($audMyGroups) {   
    foreach ($audMyGroups as $gid => $gname) {
      $members = gst_get_og_group_members($gid, 'node');
      if ($members) {
        $groups += gst_og_load_groups_from_membership($members);
      }
      //$group = og_load_entity_from_group($gid);
      //$h = og_get_hierarchy('group', $gid);
    }
    //$group_memberships = og_membership_load_multiple(array_keys($audMyGroups));
//    foreach ($group_memberships as $group_membership) {
//      $groups[$entity_type][$id][$group_membership->gid] = $group_membership->gid;
//    }      
  }

}

function gst_get_og_group_members($gid, $entity_type) {
  $query = new EntityFieldQuery();
  $result = $query
    ->entityCondition('entity_type', 'og_membership')
    ->propertyCondition('entity_type', $entity_type, '=')
    ->propertyCondition('gid', $gid, '=')
    ->execute();
  if (!empty($result['og_membership'])) {
    $group_members = og_membership_load_multiple(array_keys($result['og_membership']));
    return $group_members;
  }
  return NULL;
}

function gst_og_load_groups($gids) {
  $groups = array();
  if (is_array($gids)) {
    foreach ($gids as $gid) {
      $groups[$gid] = og_get_group('node', $gid);
    }
  } else {
    $groups[$gids] = og_get_group('node', $gids);
  }
  
  return $groups;
}

function gst_og_load_groups_from_membership($members) {
  $groups = array();
  foreach ($members as $id => $member) {    
    $group = og_get_group($member->entity_type, $member->etid);    
    if ($group) {
      $groups[$group->gid] = $group;
    }
  }
  return $groups;
}
// </editor-fold>


// <editor-fold defaultstate="collapsed" desc="GROUPSCHEDULE">

/**
 * Implementation of hook_form_alter().
 */
function gst_gstgroupschedule_form_alter(&$form, &$form_state, $form_id) {
  $lang = isset($form['language']['#value']) ? $form['language']['#value'] : 'und';
  $node = $form['#node'];
  $is_new = empty($node->nid) || !empty($node->is_new);

//  global $firephp;  //$firephp = FirePHP::getInstance(true);
//  $firephp->log($form, 'form');
//  $firephp->log($form_state, 'form_state');
  
  dfb($form, 'fa:form');
  dfb($form_state, 'fa:form_state');
  
  if ($is_new) {
    //-- ADD (NEW)
    $parent_type_value = isset($form_state['input']['field_s_parent_type']) ? $form_state['input']['field_s_parent_type'][$lang] : NULL;
    $parent_ref_value = isset($form_state['input']['field_s_parent_ref']) ? $form_state['input']['field_s_parent_ref'][$lang] : NULL;    
  } else {
    //-- EDIT (OLD)
    $parent_type_value = isset($form_state['input']['field_s_parent_type']) ? 
      $form_state['input']['field_s_parent_type'][$lang] :       
      (isset($form['field_s_parent_type'][$lang]['default_value'][0]) ? $form['field_s_parent_type'][$lang]['default_value'][0] : NULL);
      //(isset($node->field_s_parent_ref[$lang][0]) ? $node->field_s_parent_ref[$lang][0]['nid'] : NULL);
    $parent_ref_value = isset($form_state['input']['field_s_parent_ref']) ? 
      $form_state['input']['field_s_parent_ref'][$lang] :       
      (isset($form['field_s_parent_ref'][$lang]['default_value'][0]) ? $form['field_s_parent_ref'][$lang]['default_value'][0] : NULL);
  }
  
  //---- ADD & EDIT
  dfb($parent_type_value, "fa:parent_type_value");
  $fn = 'field_s_parent_type';
  $form[$fn . '_change'] = array(
    '#type' => 'button',
    '#button_type' => 'button',
    '#executes_submit_callback' => FALSE,    
    '#value' => 'Parent Type Changed',
    '#weight' => $form[$fn]['#weight'] + 0.001,
    '#limit_validation_errors' => array(), // If we don't do this, it will try to VALIDATE the form! (and gen errors - and break the JSON)
    '#attributes' => array('class' => array('hidden')),
    '#ajax' => array(
      'callback' => 'gst_gstgroupschedule_parenttype_change_js',
      //'wrapper' => 'node_gstgroupschedule_form_group_s_participating_groups',
      'wrapper' => 'edit-field-s-parent-ref-wrapper',
      'method' => 'replace',
      'effect' => 'fade',          
      ),
  );
  if (array_key_exists($fn, $form['#group_children'])) {
    $fn_group = $form['#group_children'][$fn];
    $form['#group_children'][$fn . '_change'] = $fn_group;
    $form['#groups'][$fn_group]->children[] = $fn . '_change';
  }
  $form['field_s_parent_type']['#attributes']['class'][] = 'has-ajax-button';
  $form['field_s_parent_type']['#attributes']['ajax-button-id'] = 'edit-field-s-parent-type-change';
  $form['field_s_parent_ref']['#prefix'] = '<div id="edit-field-s-parent-ref-wrapper">';
  $form['field_s_parent_ref']['#suffix'] = '</div>';
  
  
  $nodeTypes = isset($parent_type_value) && $parent_type_value != '_none' ? array($parent_type_value) : array('gstgroup', 'gstorg');
  $settings= array(
    'includeNone' => TRUE, 
    //'nodeTypes' => array('gstgroup','gstorg'),
    'nodeTypes' => $nodeTypes,
    );
  $parent_ref_options = gst_get_parentref_options($settings);
  $form['field_s_parent_ref'][$lang]['#options'] = $parent_ref_options;
  
  $settings= array(
    'includeNone' => FALSE, 
    'includeAll' => TRUE,
    'parentRef' => $parent_ref_value,
    );  
  $group_ref_options = gst_get_groupref_options($settings);

  // Test Date Field Updating \\
  //$form['field_s_last_proc_dt'][$lang][0]['#default_value']['value'] = "2011-07-01 16:57:50";  // THIS WORKS!
  //$input = drupal_array_get_nested_value($form_state['input'], $element['#parents'], $input_exists);
  $form['field_s_last_proc_dt'][$lang][0]['#title'] = "Test Update";      // Doesn't WORK
  $form['field_s_last_proc_dt'][$lang]['#title'] = "Test Update";         // Doesn't WORK  
  $form_state['field']['field_s_last_proc_dt'][$lang]['instance']['label'] = 'New Label'; // THIS WORKS!
  
  // ParentRef AjAX
  $ret = gst_add_ajax_button($form, 'field_s_parent_ref', 
    array(
      '#ajax' => array(
        'callback' => 'gst_gstgroupschedule_parentref_change_js', // (default: gst_$element_name_change_js)
        //'wrapper' => 'html_id', // (handled by targetElement)
      ),
      'targetElement' => 'field_s_group_ref',
      //'hideButton' => FALSE,
      //'noAjax' => TRUE,     
    ));
  
  // Schedule Processing
  // - Add Process Button
  $form['button_process'] = array(
    '#name' => 'button_process',
    '#type' => 'button',
    '#button_type' => 'button',
    '#executes_submit_callback' => FALSE,    
    '#value' => t('Process Schedule'),
    '#weight' => 0,
    '#limit_validation_errors' => array(), // If we don't do this, it will try to VALIDATE the form! (and gen errors - and break the JSON)
    //'#submit' => array('gst_include_form', 'gst_gstgroupschedule_process_js'),
    '#ajax' => array(
      'callback' => 'gst_gstgroupschedule_process_js',
      //'wrapper' => 'node_gstgroupschedule_form_group_s_proc',
      'method' => 'replace',
      'effect' => 'fade',      
    ),    
  );
  // - Add Process Button to Group
  $proc_group = 'group_s_proc';
  $form['#group_children']['button_process'] = $proc_group;
  $form['#groups'][$proc_group]->children[] = 'button_process';
  gst_wrap_widget($form, 'button_process');
  
  gst_wrap_widget($form, 'field_s_last_proc_dt');
  
  // Process Button Click
  dfb(isset($form_state['input'])?$form_state['input']:NULL, 'fa2:form_state-input');
  dfb(isset($form_state['values']) ? $form_state['values']:NULL, 'fa2:form_state-values');
  //dfb($form_state['triggering_element'], 'fa2:triggering_element');
  $triggering_element = isset($form_state['triggering_element']) ? $form_state['triggering_element']['#name'] : NULL;
  //dfb($triggering_element, 'triggering_element');
  if ($triggering_element) {
    switch ($triggering_element) {
      case 'button_process':
        $form['button_process']['#value'] = "Process Schedule 1";
        $form['field_s_last_proc_dt'][$lang][0]['#title'] = "Test Update";      // Doesn't WORK
        $form['field_s_last_proc_dt'][$lang]['#title'] = "Test Update";         // Doesn't WORK
        $form['field_s_last_proc_dt'][$lang][0]['#default_value']['value'] = "2011-08-01 16:57:50";
        //$form_state['input']['field_s_last_proc_dt'][$lang][0]['value']['date'] = '2011-08-01'; // WORKS
        
        //form_set_value($form['field_s_last_proc_dt'], array("2011-10-01 16:57:50"), $form_state); // Doesn't WORK - CCK/FieldAPI fields don't seem to have a "#parents" property        
        //$form_state['values']['field_s_last_proc_dt'][$lang][0]['value']['date'] = '2011-08-01';
        
        // - 
        break;
    }
  }
  
  dfb($form['button_process'], 'fa2:button_process');
  dfb($form['field_s_last_proc_dt'], 'fa2:field_s_last_proc_dt');
  
  
/* TEST SELECT  
  $form['test_select'] = array(
    '#type' => 'select',
    '#title' => t('Test Select'),
    '#options' => array(
      '' =>  '- None -',
      '1' => 'Item 1',
      '2' => 'Item 2',
    ),
    '#ajax' => array(    
      'event' => 'change', // select, autocomplete    
      'callback' => 'gst_gstgroupschedule_course_change_js',
      'wrapper' => 'edit-field-s-group-ref',
      'method' => 'replace',
      'effect' => 'fade',    
      ),
  );
*/ // TEST SELECT

/* AJAX TEST CODE 
  // Add an AJAX call after Course set
  $fn = 'field_s_parent_type';
  $form[$fn]['#ajax'] = array(    
    'event' => 'change', // select, autocomplete    
    //'path' => '/system/ajax?XDEBUG_SESSION=netbeans-xdebug',
    'callback' => 'gst_gstgroupschedule_course_change_js',
    //'wrapper' => 'node_gstgroupschedule_form_group_s_participating_groups',
    'wrapper' => 'edit-field-g-group-ref',
    'method' => 'replace',
    'effect' => 'fade',    
  );
  $form[$fn]['#process'][] = "ajax_process_form";
  $form[$fn]['#processed'] = FALSE;
  
  // Add an AJAX call after Course set
//  $form['field_s_course_ref']['#ajax'] = array(    
//    //'event' => 'blur', // textfield, textarea
//    'event' => 'change', // select, autocomplete    
//    //'event' => 'focusout', // ?
//    //'event' => 'mousedown', // submit, image, image_button
//    'callback' => 'gst_gstgroupschedule_course_change_js',
//    'wrapper' => 'edit-field-s-group-ref',
//    'method' => 'replace',
//    'effect' => 'fade',    
//  );
//  $form['field_s_course_ref']['#process'][] = "ajax_process_form";
//  $form['field_s_course_ref']['#processed'] = FALSE;

  // Add an AJAX call after Course set
//  $fn = 'field_s_parent_ref';
//  $form[$fn]['#ajax'] = array(    
//    'event' => 'change', // select, autocomplete        
//    'callback' => 'gst_gstgroupschedule_parentref_change_js',
//    'wrapper' => 'edit-field-s-group-ref',
//    'method' => 'replace',
//    'effect' => 'fade',    
//  );
//  $form[$fn]['#process'][] = "ajax_process_form";
//  $form[$fn]['#processed'] = FALSE;

  
  //dsm($form['test_select'], "FA:test select");
  //dsm($form['field_s_parent_type'], "FA:field_s_parent_type");  
*/ // AJAX TEST CODE
  
}

function gst_gstgroupschedule_parenttype_change_js($form, $form_state) {
  dfb($form_state, 'parenttype_change:form_state');
  dfb($form['field_s_parent_ref'], 'parenttype_change:field_s_parent_ref');  
  return $form['field_s_parent_ref'];
  //return $form['#groups']['group_g_participating_groups'];
}

function gst_gstgroupschedule_parentref_change_js($form, $form_state) {
  //sleep(200); // This was to test the "throbber"
  return $form['field_g_group_ref'];
}

function gst_gstgroupschedule_course_change_js($form, $form_state) {
  watchdog('gst','here');
  dfb($form_state, 'cb:form_state');
  return $form['field_g_group_ref'];
  //return $form['#groups']['group_g_participating_groups'];
}


function gst_gstgroupschedule_process_js($form, $form_state) {
  dfb($form, 'process_js:form');
  dfb($form_state, 'process_js:form_state');
  dfb($form_state['triggering_element'], 'process_js:triggering_element');
  dfb($form['button_process'], 'process_js:button_process');
  drupal_set_message("test");
  $commands = array();
  $commands[] = ajax_command_replace(NULL, theme('status_messages'));
  foreach ($form['#groups']['group_s_proc']->children as $n => $element_name) {
    //$wrapper_name = '#edit-' . str_replace('_','-',$element_name);
    $wrapper_name = $form[$element_name]['#gst_wrapper_id'];
    $commands[] = ajax_command_replace($wrapper_name, render($form[$element_name]));
  }  
  
  return array('#type' => 'ajax', '#commands' => $commands);  
}

// </editor-fold>

 
// <editor-fold defaultstate="collapsed" desc="Utility">


function gst_get_optionlist($settings) {
  // Defaults
  $settings += array(
    'includeNone' => True,
    'nodeFields' => array('nid', 'title'),    
    'orderBy' => array(
      'title' => 'ASC',
    ),
  );
  
  $includeNone = $settings['includeNone'];
  $nodeFields = $settings['nodeFields'];
  $orderBy = $settings['orderBy'];
  
  $select = db_select('node', 'n')
      ->fields('n', $nodeFields);  
  if (isset($settings['nodeTypes'])) {
    $select->condition('type', $settings['nodeTypes'], 'IN');
  }
  foreach ($orderBy as $fa => $dir) {
    $select->orderBy($fa, $dir);    
  } 
  
  $sql = $select->__toString();
  $result = $select->execute();
  $options = array();
  foreach ($result as $rec) {
    $options[$rec->nid] = $rec->type . ' | ' . $rec->title;
  }
      
  return $options;
  
}

/*
 * Returns the options list to be used by the parentref
 * 
 * @param $settings an array of settings formatted as follows:
 *    'includeNone': True | False (default: True)
 *    'nodeTypes' : array of node-types to include. eg. ('gstgroup', 'gstorg')
 * 
 * @return array of Nodes with format:
 *  [nid] -> "[Node-Type] | [Node-Title]"
 */
function gst_get_parentref_options($settings) {
  // Defaults
  $settings += array(
    'includeNone' => True,
  );
  
  $includeNone = $settings['includeNone'];
  $orderBy = array('nt_name' => 'ASC', 'title' =>'ASC');
  
  $select = db_select('node', 'n')
      ->fields('n', array('nid', 'title'));
  $n_type = $select->addField('n', 'type', 'n_type');
  $nt_alias = $select->join('node_type', 'nt', 'n.type = nt.type');
  $nt_name = $select->addField($nt_alias, 'name', 'nt_name');
  if (isset($settings['nodeTypes'])) {
    $select->condition('n.type', $settings['nodeTypes'], 'IN');
  }
  foreach ($orderBy as $fa => $dir) {
    $select->orderBy($fa, $dir);    
  } 
  
  $sql = $select->__toString();
  dfb($sql, "$sql");
  $result = $select->execute();
  $options = array();
  if ($includeNone) {
    $options['__none'] = '- None -';
  }  
  foreach ($result as $rec) {
    $options[$rec->nid] = $rec->nt_name . ' | ' . $rec->title;
  }
      
  return $options;
  
}

/*
 * Returns the options list to be used by the parentref
 * 
 * @param $settings an array of settings formatted as follows:
 *    'includeNone': True | False (default: True)
 *    'includeAll': True | False (default: True)
 *    'parentRef' : NID of Parent Ref - this determines what "groups" to display
 * @return array of Groups with format:
 *  [nid] -> "[Org-ShortName] | [Node-Title]" 
 */
function gst_get_groupref_options_unused($settings) {
  // Defaults
  $settings += array(
    'includeNone' => FALSE,
    'includeAll' => FALSE,
  );
  
  $includeNone = $settings['includeNone'];
  $includeAll = $settings['includeAll'];
  $orderBy = array('org_shname' => 'ASC', 'title' =>'ASC');
  
  $select = db_select('node', 'n')
      ->fields('n', array('nid', 'title'));
  //$n_type = $select->addField('n', 'type', 'n_type');
  $shname_alias = $select->join('o_shname', 'nt', 'n.nid = o_shname.entity_id AND n.vid = o_shname.revision_id');
  $nt_name = $select->addField($nt_alias, 'name', 'nt_name');
  if (isset($settings['nodeTypes'])) {
    $select->condition('n.type', $settings['nodeTypes'], 'IN');
  }
  foreach ($orderBy as $fa => $dir) {
    $select->orderBy($fa, $dir);    
  } 
  
  $sql = $select->__toString();
  dfb($sql, "$sql");
  return;
  $result = $select->execute();
  $options = array();
  if ($includeNone) {
    $options['__none'] = '- None -';
  }  
  foreach ($result as $rec) {
    $options[$rec->nid] = $rec->nt_name . ' | ' . $rec->title;
  }
      
  return $options;
  
}

/*
 * Returns the options list to be used by the parentref
 * 
 * @param $settings an array of settings formatted as follows:
 *    'includeNone': True | False (default: True)
 *    'includeAll': True | False (default: True)
 *    'parentRef' : NID of Parent Ref - this determines what "groups" to display
 * @return array of Groups with format:
 *  [nid] -> "[Org-ShortName] | [Node-Title]" 
 */
function gst_get_groupref_options($settings) {
  // Defaults
  $settings += array(
    'includeNone' => FALSE,
    'includeAll' => FALSE,    
  );
  
  $includeNone = $settings['includeNone'];
  $includeAll = $settings['includeAll'];
  $parentRef = $settings['parentRef'];
  $orderBy = array('org_shname' => 'ASC', 'title' =>'ASC');
  
  // Load the Entity for the Parent
  //$eParent = entity_load_single('node', $parentRef);
  //$nParent = node_load($parentRef);
  //$gParent = og_get_group('node', $parentRef);
  
  //$wrapper = entity_metadata_wrapper('entity', $parentRef);
  //$groups = og_membership_load($gParent->gid);
  
  // Load all the Groups assocciated with this parent
  //$groups = og_get_entity_groups($gParent->entity_type, $gParent);
  
  return;
  
  $qry = new EntityFieldQuery();
  
  
  $result = $query
    ->entityCondition('entity_type', 'node')    // Only Nodes
    ->entityCondition('bundle', 'gstgroup')     // Only Groups
    ->propertyCondition('status', 1)            // Published
    ->fieldCondition('field_o_shname', 'value', $parentRef, '=');
  
  
  db_select('node', 'n')
      ->fields('n', array('nid', 'title'));
  //$n_type = $select->addField('n', 'type', 'n_type');
  $shname_alias = $select->join('o_shname', 'nt', 'n.nid = o_shname.entity_id AND n.vid = o_shname.revision_id');
  $nt_name = $select->addField($nt_alias, 'name', 'nt_name');
  if (isset($settings['nodeTypes'])) {
    $select->condition('n.type', $settings['nodeTypes'], 'IN');
  }
  foreach ($orderBy as $fa => $dir) {
    $select->orderBy($fa, $dir);    
  } 
  
  $sql = $select->__toString();
  dfb($sql, "$sql");
  return;
  $result = $select->execute();
  $options = array();
  if ($includeNone) {
    $options['__none'] = '- None -';
  }  
  foreach ($result as $rec) {
    $options[$rec->nid] = $rec->nt_name . ' | ' . $rec->title;
  }
      
  return $options;
  
}


// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Ajax Utility">

/*
 * This function wraps a widget with a set of <div> tags with the id of:
 *  $element_name-wrapper
 */
function gst_wrap_widget(&$form, $widget_name, $prefix = 'edit-', $suffix = '-cbwrapper') {
  if (empty($form[$widget_name])) {
    // Element Name does not exist, abort
    watchdog('gst', "'@element_name' does not exist. Could not wrap widget.", array('@element_name' => $widget_name), WATCHDOG_WARNING);
    return FALSE;
  }
  
  $wrapper = $prefix . str_replace('_', '-', $widget_name) . $suffix;  
  $form[$widget_name]['#prefix'] = "<div id='$wrapper'>";
  $form[$widget_name]['#suffix'] = "</div>";
  $form[$widget_name]['#gst_wrapper_id'] = "#$wrapper";
  return $wrapper;
}

/*
 * This entire procedure is to solve a problem with FieldAPI/CCK node_forms
 * For some reason, if you modify a "node_form" field in the form_alter and 
 * add some #ajax settings, it doesn't process them.
 * This "wrapper" function does the work to create a hidden "button" that
 * can be placed *near* the form. Then some Javascript will be added to the
 * element that will "click" the button at the appropriate time and fire the
 * #ajax callback on it...not very pretty, but it works until I can figure
 * out how to make the #ajax work.
 * 
 * Usage:
 * 
 
  $ret = gst_add_ajax_button($form, 'field_s_parent_type', 
   array(
     '#ajax' => array(
       'callback' => 'callback_func', // (default: gst_$element_name_change_js)
       'wrapper' => 'html_id', // HTML ID to replace
       'method' => 'replace',
       'effect' => 'fade',
     ),
     'targetElement' => 'target_element', // if you want to target a specific $form[] element, just put its name here
     'wrapTarget' => TRUE, // (default: TRUE) If you want to wrap the target element in DIVs
     'hideButton' => TRUE, // if you want the button to be hidden
     'elementSuffix' => 'change', // the suffix to put at the end of the button. By default, button is named: $element_name_change
     'buttonText' => '', // Text to put on button (if it is not hidden), defaults to "$element_name changed"
     ));
 
 */
function gst_add_ajax_button(&$form, $element_name, $options = array()) {
  $options += array(
    '#ajax' => array(
      'callback' => 'gst_'.$element_name.'_change_js',
      //'wrapper' => 'edit-elementname',
      'method' => 'replace',
      'effect' => 'fade',      
    ),
    //'targetElement' => '',        // Indicates the Target Element in the form
    'wrapTarget' => TRUE,           // If a targetElement is specified, wrap the element in prefix/suffix DIV tags
    'hideButton' => TRUE,           // Hide the button (add the hidden class)
    'elementSuffix' => 'change',    // What suffix to add after "element_name" to button
    'buttonText' => "$element_name changed",
    'noAjax' => FALSE,              // If you want to turn off AJAX (for testing) set to TRUE
  );
  
  $wrapTarget = $options['wrapTarget'];
  $hideButton = $options['hideButton'];
  $eleSuffix = $options['elementSuffix'];
  $buttonText = $options['buttonText'];
  $noAjax = $options['noAjax'];
  
  if (empty($form[$element_name])) {
    // Element Name does not exist, abort
    watchdog('gst', "'@element_name' does not exist. Could not add ajax button.", array('@element_name' => $element_name), WATCHDOG_WARNING);
    return FALSE;
  }
  
  // Are we replacing a single Target Element?
  if (!empty($options['targetElement'])) {
    $targetElement = $options['targetElement'];
    if (empty($form[$targetElement])) {
      // targetElement Name does not exist, abort
      watchdog('gst', "Target Element '@targetElement' does not exist. Could not add ajax button for @element_name.", 
          array('@targetElement' => $targetElement, '@element_name' => $element_name), WATCHDOG_WARNING);
      return FALSE;      
    }
    if ($wrapTarget) {
      $wrapper = 'edit-' . str_replace('_', '-', $targetElement) . '-wrapper';
      $form[$targetElement]['#prefix'] = "<div id='$wrapper'>";
      $form[$targetElement]['#suffix'] = "</div>";      
    } else {
      $wrapper = 'edit-' . str_replace('_', '-', $targetElement);
    }
    // Set the wrapper so we can get it later
    $options['#ajax']['wrapper'] = $wrapper;
  }
  
  // Get the Wrapper
  if (empty($options['#ajax']['wrapper'])) {
    watchdog('gst', "No wrapper specified for @element_name. Could not add ajax button.", array('@element_name' => $element_name), WATCHDOG_WARNING);
    return FALSE;    
  }
  
  $buttonName = $element_name . '_' . $eleSuffix;
  $buttonId = 'edit-' . str_replace('_', '-', $buttonName);
  
  $form[$buttonName] = array(
    '#type' => 'button',
    '#button_type' => 'button',
    '#executes_submit_callback' => FALSE,    
    '#value' => $buttonText,
    '#weight' => $form[$element_name]['#weight'] + 0.001,
    '#limit_validation_errors' => array(), // If we don't do this, it will try to VALIDATE the form! (and gen errors - and break the JSON)
    '#attributes' => ($hideButton) ? array('class' => array('hidden')) : array(),
    '#ajax' => (!$noAjax) ? $options['#ajax'] : NULL,   // Get #ajax from $options array
  );
  // If there is a group, and it contains the $element_name, we need to add the button to that group
  //  so it will display correctly
  if (array_key_exists($element_name, $form['#group_children'])) {
    $en_group = $form['#group_children'][$element_name];
    $form['#group_children'][$buttonName] = $en_group;
    $form['#groups'][$en_group]->children[] = $buttonName;
  }
  
  // Now add some attributes to the $element
  $form[$element_name]['#attributes']['class'][] = 'has-ajax-button';
  $form[$element_name]['#attributes']['ajax-button-id'] = $buttonId;

  return TRUE;
}

// </editor-fold>
