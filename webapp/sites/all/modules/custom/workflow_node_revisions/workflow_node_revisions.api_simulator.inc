<?php

/**
 * @file
 * Administrative pages for configuring workflow based revisions.
 */

/**
 * Page callback. Simulate the main site API for DMS. This is just a simulation for testing purposes.
 */
function workflow_node_revisions_api_simulator() {
$api_response = array();

$op = $_SERVER['REQUEST_METHOD'];

if ($op == 'PUT' || $op == 'DELETE') {
  $putdata = fopen("php://input", "r");
  $data = stream_get_contents($putdata);
  fclose($putdata);

  parse_str($data, $post_vars);
  $node = json_decode($post_vars['node']);
}
elseif (isset($_REQUEST['node'])) {
  $node = json_decode($_REQUEST['node']);
}
else {
  $api_response['op_status'] = 'Error';
  $api_response['status_message'] = 'Error: No dataset was specified in this API call';
  $api_response['data'] = $_REQUEST;
  echo json_encode($api_response);
  return;
}

if (isset($node->field_ds_unique_id[0]->value)) {
  $dataset_unique_id = $node->field_ds_unique_id[0]->value;
}
else {
  $dataset_unique_id = FALSE;
}

switch ($op) {
  case 'POST':
    if ($dataset_unique_id) {
      $api_response['op_status'] = 'Error';
      $api_response['status_message'] = "Error: A unique ID is already specified for operation:$op in this API call. This dataset already exists and cannot be created. DATASET UNIQUE ID: $dataset_unique_id";
    }
    else {
      $dataset_unique_id = md5($node->title) . '<=>' . rand(1000000 , 1000000000);
      $api_response['op_status'] = 'Create';
      $api_response['status_message'] = 'Created the dataset with unique ID ' . $dataset_unique_id;
      $api_response['data'] = $dataset_unique_id;
    }
    break;

  case 'GET':
    if (!$dataset_unique_id) {
      $api_response['op_status'] = 'Error';
      $api_response['status_message'] = "Error: No unique ID specified for operation:$op in this API call";
    }
    else {
      $api_response['op_status'] = 'Read';
      $api_response['status_message'] = 'Read the dataset with unique ID' . $dataset_unique_id;
      $api_response['data'] = $node;
    }
    break;

  case 'PUT':
    if (!$dataset_unique_id) {
      $api_response['op_status'] = 'Error';
      $api_response['status_message'] = "Error: No unique ID specified for operation:$op in this API call";
    }
    else {
      $api_response['op_status'] = 'Update';
      $api_response['status_message'] = 'Updated the dataset with unique ID ' . $dataset_unique_id;
    }
    break;

  case 'DELETE':
    if (!$dataset_unique_id) {
      $api_response['op_status'] = 'Error';
      $api_response['status_message'] = "Error: No unique ID specified for operation:$op in this API call";
    }
    else {
      $api_response['op_status'] = 'Delete';
      $api_response['status_message'] = 'Deleted the dataset with unique ID ' . $dataset_unique_id;
    }
    break;

  default:
    $api_response['op_status'] = 'Unknown operation : ' . $op;
    $api_response['status_message'] = 'Unknown operation : ' . $op;
    $api_response['data'] = $op;
    break;
}

echo json_encode($api_response);
return;
}
