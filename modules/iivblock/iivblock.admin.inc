<?php

/**
 * @file
 * Functions regarding administration of IIV Blocks.
 */

/**
 * Form handler for the site wide IIV Block admin settings.
 */
function iivblock_admin_settings_form() {
  $config = config('iivblock.settings');
  $form['#config'] = 'iivblock.settings';
  $form['iivblock_image_path'] = array(
    '#type' => 'textfield',
    '#title' => t('IIV Block image directory'),
    '#description' => t('Subdirectory in the file upload directory where images will be stored.'),
    '#default_value' => $config->get('iivblock_image_path'),
    '#maxlength' => 255,
  );
  $form['iivblock_max_file_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum file size'),
    '#description' => t('Specify the size limit that applies to each image. Enter a value like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes) in order to restrict the allowed file size. If you leave this empty the file sizes will be limited only by PHP\'s maximum post and file upload sizes (current limit <strong>%limit</strong>).', array('%limit' => format_size(file_upload_max_size()))),
    '#default_value' => $config->get('iivblock_max_file_size'),
    '#maxlength' => 15,
  );
  $form['iivblock_max_dimensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum dimensions'),
    '#description' => t('The maximum allowed image size expressed as WIDTHxHEIGHT (e.g. 640x480). Set to 0 for no restriction. If a larger image is uploaded, it will be resized to reflect the given width and height.'),
    '#default_value' => $config->get('iivblock_max_dimensions'),
    '#maxlength' => 15,
  );
  return system_settings_form($form);
}

/**
 * Validation handler for the site wide IIV Block admin settings.
 */
function iivblock_admin_settings_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  if (!preg_match('/^[a-zA-Z0-9_-]*$/', $values['iivblock_image_path'])) {
    form_set_error('iivblock_image_path', t('The directory name contains invalid characters. Only alphanumeric characters, hyphens and underscores are allowed.'));
  }
  foreach (array('iivblock_max_file_size') as $size) {
    if (!empty($values[$size]) && !is_numeric(parse_size($values[$size]))) {
      form_error($size, t('The "@field" option must contain a valid value. You can either leave the text field empty or enter a string like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes).', array('@field' => t('Maximum upload size per file'))));
    }
  }
  foreach (array('iivblock_max_dimensions') as $resolution) {
    if (!empty($values[$resolution]) && !preg_match('/^[0-9]+x[0-9]+$/', $values[$resolution])) {
      form_set_error($resolution, t('Please specify a resolution in the format WIDTHxHEIGHT (e.g. 640x480).'));
    }
  }
}

/**
 * Form handler to delete an IIV Block.
 */
function iivblock_block_delete($form, &$form_state, $machine_name) {
  $iivblock = config_get('iivblock.block.' . $machine_name);
  $form['machine_name'] = array(
    '#type' => 'hidden',
    '#value' => $iivblock['machine_name'],
  );
  $form['image'] = array(
    '#type' => 'hidden',
    '#value' => $iivblock['image'],
  );
  $confirm_message = t('Are you sure you want to delete the block %name?', array('%name' => $iivblock['label']));
  return confirm_form($form, $confirm_message, 'admin/structure/iivblock/list', '', t('Delete'), t('Cancel'));
}

/**
 * Form submission handler to delete an IIV Block.
 */
function iivblock_block_delete_submit($form, &$form_state) {
  if (!empty($form_state['values']['image'])) {
    $file = file_load($form_state['values']['image']);
    if ($file) {
      file_usage_delete($file, 'iivblock');
    }
  }
  config('iivblock.block.' . $form_state['values']['machine_name'])->delete();
  backdrop_set_message(t('The IIV Block %name has been removed.', array('%name' => $form_state['values']['machine_name'])));
  $form_state['redirect'] = 'admin/structure/iivblock/list';
}

/**
 * Callback to provide a listing page with all IIV Blocks.
 */
function iivblock_list() {
  $header = array(
    t('Admin label'),
    t('Image'),
    t('Text preview'),
    t('Required for'),
    t('Action'),
  );
  $rows = array();
  $config_names = config_get_names_with_prefix('iivblock.block.');
  $iivblock_usage = iivblock_layout_usage();
  $parents = array();
  foreach ($config_names as $config_name) {
    $parent = config_get($config_name, 'parent');
    if ($parent && $parent != 'default') {
      $parents[] = $parent;
    }
  }

  foreach ($config_names as $config_name) {
    $block = config_get($config_name);
    $preview_text = t('No text set');
    if (!empty($block['title'])) {
      $plain = strip_tags($block['title']);
      $preview_text = $plain;
    }
    elseif (!empty($block['content'])) {
      $plain = strip_tags($block['content']);
      $preview_text = substr($plain, 0, 30);
    }
    $image = '-';
    if (!empty($block['image'])) {
      $file = file_load($block['image']);
      if ($file) {
        $variables = array(
          'uri' => $file->uri,
          'width' => isset($file->width) ? $file->width : NULL,
          'height' => isset($file->height) ? $file->height : NULL,
          'alt' => '',
          'attributes' => array(),
        );
        if ($file->filemime != 'image/svg+xml') {
          $variables['style_name'] = 'thumbnail';
          $image = theme('image_style', $variables);
        }
        else {
          $variables['width'] = 100;
          $image = theme('image', $variables);
        }
      }
    }
    $required_for = '-';
    if (array_key_exists($block['machine_name'], $iivblock_usage)) {
      $current_usage = $iivblock_usage[$block['machine_name']];
      $admin_path = 'admin/structure/layouts/manage/' . $current_usage['layout'];
      $options = array(
        'fragment' => 'layout-editor-region-' . $current_usage['region'],
      );
      $linktext = $current_usage['layout_title'] . ' / ' . $current_usage['region'];
      $required_for = l($linktext, $admin_path, $options);
    }
    $action = '-';
    if ($required_for == '-') {
      if (!in_array($block['machine_name'], $parents)) {
        $path = 'admin/structure/iivblock/' . $block['machine_name'] . '/delete';
        $link_options = array('attributes' => array('class' => array('button')));
        $action = l(t('Delete'), $path, $link_options);
      }
      else {
        $required_for = t('Has dependent block');
      }
    }
    $rows[] = array(
      $block['label'],
      $image,
      $preview_text,
      $required_for,
      $action,
    );
  }
  $markup = '<p>' . t('Please note:') . '</p>';
  $markup .= theme('item_list', array(
    'items' => array(
      t('You can only delete IIV Blocks not in use in any layout and not parent of a copied block in use.'),
      t('You can create new IIV Blocks by adding an IIV Block to a layout.'),
    )));
  $markup .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'empty' => t('No IIV Blocks (yet).'),
  ));
  return $markup;
}

/**
 * Access callback function for iivblock delete form.
 */
function iivblock_delete_access($machine_name) {
  $block = config_get('iivblock.block.' . $machine_name);
  if ($block && user_access('administer layouts')) {
    $config_names = config_get_names_with_prefix('iivblock.block.');
    $iivblock_usage = iivblock_layout_usage();
    $parents = array();
    foreach ($config_names as $config_name) {
      $parent = config_get($config_name, 'parent');
      if ($parent && $parent != 'default') {
        $parents[] = $parent;
      }
    }
    if (in_array($block['machine_name'], $parents)) {
      // IIV Block needed by a copy block.
      return FALSE;
    }
    if (array_key_exists($block['machine_name'], $iivblock_usage)) {
      // IIV Block is in use in a layout.
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

/**
 * Helper function to get all IIV Block usages in layouts.
 */
function iivblock_layout_usage() {
  $iivblocks_in_use = array();
  module_load_include('inc', 'layout', 'layout.admin');
  @$usage = layout_get_block_usage();
  if (isset($usage['iivblock'])) {
    $delta = $usage['iivblock'];
    foreach ($delta as $layouts) {
      foreach ($layouts as $layout_name => $layout) {
        foreach ($layout as $region_name => $region) {
          foreach ($region as $iivblock) {
            $iivblocks_in_use[$iivblock->settings['machine_name']] = array(
              'layout' => $layout_name,
              'layout_title' => $iivblock->layout_title,
              'region' => $region_name,
            );
          }
        }
      }
    }
  }
  return $iivblocks_in_use;
}
