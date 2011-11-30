<?php

function dms_customizations_expand_checkboxes($element) {
$value = is_array($element['#value']) ? $element['#value'] : array();
var_dump($value);
}

$element = 'test';//array('#value' => array('test'=>'success'));
dms_customizations_expand_checkboxes($element);


