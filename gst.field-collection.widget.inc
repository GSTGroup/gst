<?php
// $Id$ 
/** 
 * @file 
 * GST Field-Collection Widget
 * 
 */

//ADFTODO: See field_example.module for details on how to add the Field and Widget
//ADFTODO: I need to finish this by doing the following: create a new "field" type of "field-collection-reference"
//ADFTODO: I need to create a new widget of type "field-collection-ref-select" used for selecting fc-refs
//ADFTODO: I should move the entire set of code to a new module called: field_collection_references

/**************************************************************************
 * Field Type API: Widget
 *
 * The widget is the form element used to receive input from the user
 * when the field is being populated.
 **************************************************************************/

/**
 * Implements hook_field_widget_info().
 *
 * One widget provided.
 * - A simple widget for creating Field-Collection References
 *
 */
function gst_field_widget_info() {
  return array(
    'gst_field_collection_select' => array(
      'label' => t('Field-Collection Select'),
      'field types' => array('text'),
    ),
  );
}

/**
 * Implements hook_field_widget_form().
 *
 * Three different forms are provided, for the three widget types.
 *
 * The 'field_example_colorpicker' and 'field_example_text' are essentially
 * the same, but field_example_colorpicker adds a javascript colorpicker
 * helper.
 *
 * field_example_3text displays three text fields, one each for red, green,
 * and blue. However, the field type defines a single text column,
 * rgb, which needs an HTML color spec. Define an element validate
 * handler that converts our r, g, and b fields into a simulated single
 * 'rgb' form element.
 */
function gst_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  switch ($instance['widget']['type']) {
    case 'gst_field_collection_select':
      $element['nid'] += array(
        '#type' => 'select',
        '#default_value' => isset($items[$delta]['nid']) ? $items[$delta]['nid'] : NULL,
        '#options' => array(
          'a' => t('Item a'), 
          'b' => t('Item b'), 
          'c' => t('Item c'),
          ),        
      );
      
      $element['value'] += array(
        '#type' => 'select',
        '#default_value' => isset($items[$delta]['value']) ? $items[$delta]['value'] : NULL,
        '#options' => array(
          '1' => t('Item 1'), 
          '2' => t('Item 2'), 
          '3' => t('Item 3'),
          ),
      );
      break;
  }

  return $element;

}