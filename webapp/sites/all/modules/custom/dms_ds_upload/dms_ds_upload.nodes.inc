<?php
/**
 * @file
 * Part of module to deal with creating node from file data.
 */
 
require_once 'dms_ds_upload.fields.inc';
require_once 'dms_ds_upload.files.inc';

/**
 * Add dataset nodes from uploaded file.
 */
function dms_ds_upload_add_dataset($filename = '') {

  //get records from the file content
  $data_from_file = array();
  $path = $_SERVER['DOCUMENT_ROOT'] . base_path() . file_directory_path();
  $filename = $path . '/' . $filename;

  $data_from_file = dms_ds_upload_parse_file($filename);

  //delete file after done.
  file_delete($filename);
  
  $num_of_records = count($data_from_file);
  $num_of_nodes = 0;
  
  if ($num_of_records) {
    //walk thru all valid fields
    $all_fields = dms_ds_upload_get_fields('dataset');
    //create one node for each record
    for ($i = 0; $i < $num_of_records; $i++) {
      $nid = dms_ds_upload_create_node($data_from_file[$i], $all_fields);
      if (!empty($nid)) {
        $num_of_nodes++;
      }
    }
  }
  
  //Now node(s) added, redirecting user to appropriate page.
  if ($num_of_nodes == 0) { //either bad entries, or 0 entries.
    drupal_set_message(t('Uploaded file is not valid. ' .
                         'No dataset has been created.'
                        ));
    drupal_goto('node/add/dataset');
  }
  elseif ($num_of_records == 1) {
    //one entry and it is good. redirect to node view page.
    drupal_set_message(t("New dataset has been created with uploaded file."));
    drupal_goto('node/' . $nid . '/edit');
  }
  else {
    //multiple entries and at least one good entry.
    //redirect to home with content listing.
    drupal_set_message(t('@count new datasets have been created with uploaded file.',
                          array('@count' => $num_of_nodes)
                        ));
    drupal_goto('');
  }

}

/**
 * Put data into fields and we got a node.
 */
function dms_ds_upload_create_node($data = array(), $fields = array()) {

  global $user;
  // Insert new data:
  $node->type = 'dataset';
  module_load_include('inc', 'node', 'node.pages');
  $type = node_get_types('type', $node->type);
  node_object_prepare($node);
  $node->uid = $user->uid;
  //commented out. let us use default publishing status
  //$node->status = 0;

  $supported_types = $GLOBALS['dms_ds_upload_supported_types'];
  foreach ($supported_types as $type) {
    if (isset($fields[$type])) {
      dms_ds_upload_insert_field($node, $fields[$type], $data, $type);
    }
  }
  
  node_save($node);
  
  //we are done already. but let caller know what nid we created.
  return $node->nid;
}

/**
 * Treat each type of field differently when it comes to inserting data.
 */
function dms_ds_upload_insert_field(&$node = NULL, $fields, $data = array(), $field_type = '') {
  global $user;

  //$node can be a node, or a node element for recursive flexifield.
  $is_node = is_object($node);


  switch ($field_type) {

    case 'datestamp':
      $default_timezone = date_default_timezone_get();
      $user_timezone = $user->timezone_name;
      //use user timezone temporarily, so strtotime will do things right.
      date_default_timezone_set($user_timezone);
      foreach ($fields as $field) {
        $value = dms_ds_upload_clean_input($data[$field['fieldname']]);
        if (!is_null($value)) {
          $value = strtotime($value);
        }
        if ($is_node) {
          $node->{'field_' . $field['fieldname']}[0]['value'] = $value;
        }
        else {
          $node['field_' . $field['fieldname']][0]['value'] = $value;
        }
      }
      //revert back to original timezone
      date_default_timezone_set($default_timezone);
      break;

    case 'nodereference':
      foreach ($fields as $field) {
        $value = dms_ds_upload_clean_input($data[$field['fieldname']]);
        //what is the content_type this referenced to?
        $child_content_type = $field['child_fieldnames'][0];
        //give me an array of valid entries in the format of ($node_title => $nid) for all $child_content_type nodes
        $valid_entries = dms_ds_upload_get_nodereference_entries($child_content_type);
        if ($is_node) {
          $node->{'field_' . $field['fieldname']}[0]['nid'] = $valid_entries[$value];
        }
        else {
          $node['field_' . $field['fieldname']][0]['nid'] = $valid_entries[$value];
        }
      }
      break;
    
    case 'link':
      foreach ($fields as $field) {
        $value = dms_ds_upload_clean_input($data[$field['fieldname']]);
        if ($is_node) {
          $node->{'field_' . $field['fieldname']}[0]['url'] = $value;
        }
        else {
          $node['field_' . $field['fieldname']][0]['url'] = $value;
        }
      }
      break;

    case 'flexifield':
      foreach ($fields as $field) {
        $value = dms_ds_upload_clean_input($data[$field['fieldname']]);
        //what node type does user choose?
        $type_chosen = $value;
        //machine name for this type?
        $query = "SELECT type from {node_type} WHERE name ='%s'";
        $type_chosen = db_result(db_query($query, $type_chosen));
        //$type_chosen got to be a valid type, because drupal is taking care of that.
        //but if we really want to do a validatin, here is how to do it.
        if (!in_array($type_chosen, $field['child_fieldnames'])){
          continue;
        }

        if ($is_node) {
          $node->{'field_' . $field['fieldname']}[0]['type'] = $type_chosen;
        }
        else {
          $node['field_' . $field['fieldname']][0]['type'] = $type_chosen;
        }

        //now deal with flexifield child fields  
        $child_fields = dms_ds_upload_get_fields($type_chosen);
        if ($is_node) {
          if (!isset($node->{'field_' . $field['fieldname']}[0]['value'])) {
            $node->{'field_' . $field['fieldname']}[0]['value'] = array();
          }
          $child_node = &$node->{'field_' . $field['fieldname']}[0]['value'];
        }
        else {
          if (!isset($node['field_' . $field['fieldname']][0]['value'])) {
            $node['field_' . $field['fieldname']][0]['value'] = array();
          }
          $child_node = &$node['field_' . $field['fieldname']][0]['value'];
        }
        $supported_types = $GLOBALS['dms_ds_upload_supported_types'];
        foreach ($supported_types as $type) {
          if (isset($child_fields[$type])) {
            dms_ds_upload_insert_field($child_node, $child_fields[$type], $data, $type);
          }
        }
      }
      break;
      
    default: //text, number_integer...
      foreach ($fields as $field) {
        $value = dms_ds_upload_clean_input($data[$field['fieldname']]);
        if ($is_node) {
          $node->{'field_' . $field['fieldname']}[0]['value'] = $value;
        }
        else {
          $node['field_' . $field['fieldname']][0]['value'] = $value;
        }
      }

  }
}

/**
 * Retrieve readable data from node and put into array.
 */
function dms_ds_upload_retrieve_node($node = NULL, $fields = array()) {
  global $user;
  
  $query = "SELECT * from {content_type_dataset} WHERE nid=%d";
  $result = db_query($query, $node->nid);
  $data = db_fetch_array($result);

  $ret = array();

  $supported_types = $GLOBALS['dms_ds_upload_supported_types'];
  foreach ($supported_types as $type) {
    if (isset($fields[$type])) {
      dms_ds_upload_retrieve_field($ret, $fields[$type], $data, $type);
    }
  }
  
  return $ret;
}

/**
 * Treat each type of field differently when it comes to retrieving data.
 */
function dms_ds_upload_retrieve_field(&$ret = array(), $fields, $data = array(), $field_type = '') {
  global $user;
  
  //is the data from a node?
  //if so, we are dealing with first level node field.
  //otherwise it is derivative fields from unserialize flexifield value.
  $is_from_node = $data['nid'];

  switch ($field_type) {
    
    case 'datestamp':
      $default_timezone = date_default_timezone_get();
      $user_timezone = $user->timezone_name;
      //use user timezone temporarily, so date will do things right.
      date_default_timezone_set($user_timezone);
      foreach ($fields as $field) {
        if ($is_from_node) {
          if ($data['field_' . $field['fieldname'] . '_value']) {
            $ret[$field['fieldname']] = date('Y-m-d H:i', $data['field_' . $field['fieldname'] . '_value']);
          }
        }
        else {
          if ($data['field_' . $field['fieldname']][0]['value']) {
            $ret[$field['fieldname']] = date('Y-m-d H:i', $data['field_' . $field['fieldname']][0]['value']);
          }
        }

      }
      //revert back to original timezone
      date_default_timezone_set($default_timezone);
      break;


    case 'nodereference':
      foreach ($fields as $field) {
        if ($is_from_node) {
          $node_id = $data['field_' . $field['fieldname'] . '_nid'];
        }
        else {
          $node_id = $data['field_' . $field['fieldname']][0]['nid'];
        }
        $child_node_title ='';
  
        //what is the content_type this referenced to?
        $child_content_type = $field['child_fieldnames'][0];
  
        //give me an array of valid entries in the format of ($node_title => $nid) for all $child_content_type nodes
        $valid_entries = dms_ds_upload_get_nodereference_entries($child_content_type);
        foreach ($valid_entries as $key => $value) {
          if ($value == $node_id) {
            $child_node_title = $key;
            break;
          }
        }
        $ret[$field['fieldname']] = $child_node_title;
      }
      break;
    
    case 'flexifield':
      foreach ($fields as $field) {
        //what node type in the record?
        if ($is_from_node) {
          $type_chosen = $data['field_' . $field['fieldname'] . '_type'];
        }
        else {
          $type_chosen = $data['field_' . $field['fieldname']][0]['type'];
        }

        if (empty($type_chosen)) {
          continue;
        }
        //label name for this type?
        $query = "SELECT name from {node_type} WHERE type ='%s'";
        $type_name = db_result(db_query($query, $type_chosen));
        $ret[$field['fieldname']] = $type_name;
  
        $child_fields = dms_ds_upload_get_fields($type_chosen);
        if ($is_from_node) {
          $child_data = unserialize($data['field_' . $field['fieldname'] . '_value']);
        }
        else {
          $child_data = $data['field_' . $field['fieldname']][0]['value'];
        }
        $supported_types = $GLOBALS['dms_ds_upload_supported_types'];
        foreach ($supported_types as $type) {          
          if (isset($child_fields[$type])) {
            dms_ds_upload_retrieve_field($ret, $child_fields[$type], $child_data, $type);
          }
        }
      }
      break;

    case 'link':
      foreach ($fields as $field) {
        if ($is_from_node) {
          $ret[$field['fieldname']] = $data['field_' . $field['fieldname'] . '_value'];
        }
        else {
          $ret[$field['fieldname']] = $data['field_' . $field['fieldname']][0]['url'];
        }
      }
      break;
      
    default: //text, number_integer...
      foreach ($fields as $field) {
        if ($is_from_node) {
          $ret[$field['fieldname']] = $data['field_' . $field['fieldname'] . '_value'];
        }
        else {
          $ret[$field['fieldname']] = $data['field_' . $field['fieldname']][0]['value'];
        }
      }

  } 
}

/**
 * Make input data NULL if we dont like it.
 */
function dms_ds_upload_clean_input($data = NULL) {
  if ($data === "" || $data === array()) {
    return NULL;
  }
  else {
    return $data;
  }
}