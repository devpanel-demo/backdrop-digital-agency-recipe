<?php
/**
 * @file
 * Admin page callbacks for the Image Grid Block module.
 */

/**
 * Form handler for the site wide Image Grid Block admin settings.
 */
function image_grid_block_admin_settings_form() {
  $config = config('image_grid_block.settings');
  $form['#config'] = 'image_grid_block.settings';
  $form['image_grid_block_image_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Block image directory'),
    '#description' => t('Subdirectory in the file upload directory where images will be stored.'),
    '#default_value' => $config->get('image_grid_block_image_path'), 
    '#maxlength' => 255,
  );
  $form['image_grid_block_max_file_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum file size'),
    '#description' => t('Specify the size limit that applies to each image. Enter a value like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes) in order to restrict the allowed file size. If you leave this empty the file sizes will be limited only by PHP\'s maximum post and file upload sizes (current limit <strong>%limit</strong>).', array('%limit' => format_size(file_upload_max_size()))),
    '#default_value' => $config->get('image_grid_block_max_file_size'),
    '#maxlength' => 15,
  );
  $form['image_grid_block_max_dimensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum dimensions'),
    '#description' => t('The maximum allowed image size expressed as WIDTHxHEIGHT (e.g. 640x480). Set to 0 for no restriction. If a larger image is uploaded, it will be resized to reflect the given width and height.'),
    '#default_value' => $config->get('image_grid_block_max_dimensions'),
    '#maxlength' => 15,
  );
  return system_settings_form($form);
}

/**
 * Validation handler for the site wide Image Grid Block admin settings.
 */
function image_grid_block_admin_settings_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  if (!preg_match('/^[a-zA-Z0-9_-]*$/', $values['image_grid_block_image_path'])) {
    form_set_error('image_grid_block_image_path', t('The directory name contains invalid characters. Only alphanumeric characters, hyphens and underscores are allowed.'));
  }
  foreach (array('image_grid_block_max_file_size') as $size) {
    if (!empty($values[$size]) && !is_numeric(parse_size($values[$size]))) {
      form_error($size, t('The "@field" option must contain a valid value. You can either leave the text field empty or enter a string like "512" (bytes), "80K" (kilobytes) or "50M" (megabytes).', array('@field' => t('Maximum upload size per file'))));
    }
  }
  foreach (array('image_grid_block_max_dimensions') as $resolution) {
    if (!empty($values[$resolution]) && !preg_match('/^[0-9]+x[0-9]+$/', $values[$resolution])) {
      form_set_error($resolution, t('Please specify a resolution in the format WIDTHxHEIGHT (e.g. 640x480).'));
    }
  }
}

/**
 * Form handler to delete an Image Grid Block.
 */
function image_grid_block_block_delete($form, &$form_state, $machine_name) {
  $image_grid_block = config_get('image_grid_block.block.' . $machine_name);
  $form['machine_name'] = array(
    '#type' => 'hidden',
    '#value' => $image_grid_block['machine_name'],
  );
  $form['image_1'] = array(
    '#type' => 'hidden',
    '#value' => $image_grid_block['image_1'],
  );
    $form['image_2'] = array(
    '#type' => 'hidden',
    '#value' => $image_grid_block['image_2'],
  );
  $form['image_3'] = array(
    '#type' => 'hidden',
    '#value' => $image_grid_block['image_3'],
  );
  $confirm_message = t('Are you sure you want to delete the block %name?', array('%name' => $image_grid_block['label']));
  return confirm_form($form, $confirm_message, 'admin/structure/image_grid_block/list', '', t('Delete'), t('Cancel'));
}

/**
 * Form submission handler to delete an Image Grid Block.
 */
function image_grid_block_block_delete_submit($form, &$form_state) {
  if (!empty($form_state['values']['image'])) {
    $file = file_load($form_state['values']['image']);
    if ($file) {
      file_usage_delete($file, 'image_grid_block');
    }
  }
  config('image_grid_block.block.' . $form_state['values']['machine_name'])->delete();
  backdrop_set_message(t('The Image Grid Block %name has been removed.', array('%name' => $form_state['values']['machine_name'])));
  $form_state['redirect'] = 'admin/structure/image_grid_block/list';
}

/**
 * Callback to provide a listing page with all Image Grid Block.
 */
function image_grid_block_list() {
  $header = array(
    t('Admin label'),
    t('Image 1'),
    t('Image 2'),
    t('Image 3'),
    t('Text preview'),
    t('Required for'),
    t('Action'),
  );
  $rows = array();
  $config_names = config_get_names_with_prefix('image_grid_block.block.');
  $image_grid_block_usage = image_grid_block_layout_usage();
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
    $image_1 = '-';
    if (!empty($block['image_1'])) {
      $file = file_load($block['image_1']);
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
          $image_1 = theme('image_style', $variables);
        }
        else {
          $variables['width'] = 100;
          $image_1 = theme('image_1', $variables);
        }
      }
    }
    $image_2 = '-';
    if (!empty($block['image_2'])) {
      $file = file_load($block['image_2']);
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
          $image_2 = theme('image_style', $variables);
        }
        else {
          $variables['width'] = 100;
          $image_2 = theme('image_2', $variables);
        }
      }
    }
    $image_3 = '-';
    if (!empty($block['image_3'])) {
      $file = file_load($block['image_3']);
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
          $image_3 = theme('image_style', $variables);
        }
        else {
          $variables['width'] = 100;
          $image_3 = theme('image_3', $variables);
        }
      }
    }
    $required_for = '-';
    if (array_key_exists($block['machine_name'], $image_grid_block_usage)) {
      $current_usage = $image_grid_block_usage[$block['machine_name']];
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
        $path = 'admin/structure/image_grid_block/' . $block['machine_name'] . '/delete';
        $link_options = array('attributes' => array('class' => array('button')));
        $action = l(t('Delete'), $path, $link_options);
      }
      else {
        $required_for = t('Has dependent block');
      }
    }
    $rows[] = array(
      $block['label'],
      $image_1,
      $image_2,
      $image_3,
      $preview_text,
      $required_for,
      $action,
    );
  }
  $markup = '<p>' . t('Please note:') . '</p>';
  $markup .= theme('item_list', array(
    'items' => array(
      t('You can only delete Image Grid Blocks not in use in any layout and not parent of a copied block in use.'),
      t('You can create new Image Grid Blocks by adding an Image Grid Block to a layout.'),
    )));
  $markup .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'empty' => t('No Image Grid Blocks (yet).'),
  ));
  return $markup;
}

/**
 * Access callback function for Image Grid Block delete form.
 */
function image_grid_block_delete_access($machine_name) {
  $block = config_get('image_grid_block.block.' . $machine_name);
  if ($block && user_access('administer layouts')) {
    $config_names = config_get_names_with_prefix('image_grid_block.block.');
    $image_grid_block_usage = image_grid_block_layout_usage();
    $parents = array();
    foreach ($config_names as $config_name) {
      $parent = config_get($config_name, 'parent');
      if ($parent && $parent != 'default') {
        $parents[] = $parent;
      }
    }
    if (in_array($block['machine_name'], $parents)) {
      // Image Grid Block needed by a copy block.
      return FALSE;
    }
    if (array_key_exists($block['machine_name'], $image_grid_block_usage)) {
      // Image Grid Block is in use in a layout.
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

/**
 * Helper function to get all Image Grid Block usages in layouts.
 */
function image_grid_block_layout_usage() {
  $image_grid_block_in_use = array();
  module_load_include('inc', 'layout', 'layout.admin');
  @$usage = layout_get_block_usage();
  if (isset($usage['image_grid_block'])) {
    $delta = $usage['image_grid_block'];
    foreach ($delta as $layouts) {
      foreach ($layouts as $layout_name => $layout) {
        foreach ($layout as $region_name => $region) {
          foreach ($region as $image_grid_block) {
            $image_grid_block_in_use[$image_grid_block->settings['machine_name']] = array(
              'layout' => $layout_name,
              'layout_title' => $image_grid_block->layout_title,
              'region' => $region_name,
            );
          }
        }
      }
    }
  }
  return $image_grid_block_in_use;
}