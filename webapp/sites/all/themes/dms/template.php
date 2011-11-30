<?php
function dms_breadcrumb($breadcrumb) {
  if (!empty($breadcrumb)) {
    $breadcrumb[] = l(drupal_get_title(), $_GET['q']);
    return '<div class="breadcrumb">'. implode(' » ', $breadcrumb) .'</div>';
  }
}

function dms_preprocess_page(&$vars) {
  // Remove undesired local task tabs.
  // This first example removes the Users tab from the Search page.
  dms_removetab('Workflow', $vars);
}

// Remove undesired local task tabs.
// Related to yourthemename_removetab() in yourthemename_preprocess_page().
function dms_removetab($label, &$vars) {
  $tabs = explode("\n", $vars['tabs']);
  $vars['tabs'] = '';

  foreach ($tabs as $tab) {
    if (strpos($tab, '>' . $label . '<') === FALSE) {
      $vars['tabs'] .= $tab . "\n";
    }
  }
}
?>