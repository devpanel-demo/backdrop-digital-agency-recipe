<?php

/**
 *  This class contains code to handle shared functionality with Blocks.
 */
class SmartBlocksBase extends Block {

  protected $default_admin_title = '';

  /**
   * @var array A list of form fields that should be added to the block settings form.
   */
  public $fields = [];

  /**
   * @var array A list of field api fields that can be used for block titles.
   */
  protected $title_field_types = [
    'text',
    'list_text',
  ];

  /**
   * {@inheritdoc}
   */
  function __construct($plugin_name, array $data) {
    parent::__construct($plugin_name, $data);

    $this->settings += ['title_field' => ''];

    foreach ($this->fields as $field_name => $field_info) {
      switch ($field_info['type']) {
        case 'text_format':
          $this->settings += [
            $field_name => '',
            $field_name . '_format' => NULL,
          ];
          break;

        case 'image':
          $this->settings += array(
            $field_name => '',
            $field_name . '_path' => '',
            $field_name . '_field' => '',
          );
      }
    }



  }

  /**
   *  Sets title text on draggable block panel in Layout builder.
   */
  function getAdminTitle() {
    if (!empty($this->settings['admin_label'])) {
      return check_plain($this->settings['admin_label']);
    }

    $title = $this->getTitle();
    if (!empty($title)) {
      return check_plain($title);
    }

    return check_plain($this->default_admin_title);
  }

  /**
   *  Sets block subject on block view.
   */
  function getTitle() {
    return isset($this->settings['title']) ? check_plain($this->settings['title']) : '';
  }

  /**
   * Returns the rendered content of this block.
   *
   * @return string
   */
  function getContent() {
    if (!empty($this->settings['content'])) {
      return check_markup(token_replace($this->settings['content']), $this->settings['format'], '', TRUE);
    }
  }

  /**
   *  Builds the block's configuration form.
   */
  function form(&$form, &$form_state) {
    parent::form($form, $form_state);
    $form['#attached']['js'][] = backdrop_get_path('module', 'layout') . '/js/layout.admin.js';

    // Make the wrapper for title elements into a fieldset.
    $form['title_display']['#type'] = 'fieldset';

    // Add more title options.
    $form['title_display']['title_display']['#options']['page_title'] = t('Page title');
    $form['title_display']['title_display']['#options']['field'] = t('Field');

    // Add the ability to pull the title from a field.
    $form['title_display']['title_field'] = [
      '#type' => 'select',
      '#title' => t('Title field'),
      '#default_value' => $this->settings['title_field'],
      '#description' => t('Select a field to use for the title of this block. If the field is available on a page, this will override other title options for this block.'),
      '#options' => array_merge(
        ['' => t('None')],
        $this->getFieldApiFields($this->title_field_types)
      ),
    ];

    // Handle the custom fields.
    foreach ($this->fields as $field_name => $field_info) {
      switch ($field_info['type']) {
        case 'text_format':
          $form[$field_name] = [
            '#type' => 'text_format',
            '#title' => $field_info['label'],
            '#default_value' => $this->settings[$field_name],
            '#format' => $this->settings[$field_name .'_format'],
            '#editor_uploads' => TRUE,
            '#rows' => 5,
          ];
          break;

        case 'image':
          $form[$field_name . '_wrapper'] = [
            '#title' => $field_info['label'],
            '#type' => 'fieldset',
            '#tree' => FALSE,
          ];

          $form[$field_name . '_wrapper'][$field_name . '_field'] = [
            '#type' => 'select',
            '#description' => t('Select an image field to pull from the entity on the current page. If available, this will override the default image provided below.'),
            '#default_value' => $this->settings[$field_name . '_field'],
            '#options' => array_merge(
              ['' => t('None')],
              $this->getFieldApiFields(['image'])
            ),
          ];

          // Add an image upload field.
          $supported_extensions = image_get_supported_extensions();
          $upload_validators = array(
            'file_validate_extensions' => array(implode(' ', $supported_extensions)),
          );
          $upload_description = theme('file_upload_help', array(
            'upload_validators' => $upload_validators,
          ));
          $form[$field_name . '_wrapper']['image'] = array(
            '#type' => 'managed_file',
            '#description' => $upload_description,
            '#default_value' => $this->settings['image'],
            '#upload_location' => 'public://hero/',
            '#browser_view' => 'image_library',
            '#upload_validators' => $upload_validators,
          );
          // Store the image path separately.
          $form[$field_name . '_wrapper'][$field_name . '_path'] = array(
            '#type' => 'value',
            '#value' => $this->settings[$field_name . '_path'],
          );
          break;

        default:
          backdrop_set_message(t('There is no handling for fields of type @type.', ['@type' => $type]), 'warning');
      }
    }
  }

  /**
   * Validates the form settings.
   */
  function formValidate($form, &$form_state) {
    parent::formValidate($form, $form_state);
  }

  /**
   * Submit handler to save the form settings.
   */
  function formSubmit($form, &$form_state) {
    parent::formSubmit($form, $form_state);

    $this->settings['title_field'] = $form_state['values']['title_field'];

    foreach ($this->fields as $field_name => $field_info) {
      switch ($field_info['type']) {
        case 'text_format':
          $this->settings[$field_name] = $form_state['values'][$field_name]['value'];
          $this->settings[$field_name . '_format'] = $form_state['values'][$field_name]['format'];
          break;

        case 'image':
          if (!empty($form_state['values'][$field_name])) {
            $file = file_load($form_state['values'][$field_name]);
            if ($form_state['values'][$field_name] != $form[$field_name . '_wrapper'][$field_name]['#default_value']) {
              // Record that the layout module is using this file. Also marks permanent.
              // But only if the value has changed.
              file_usage_add($file,'layout', 'block', $file->fid);
              if (!empty($form[$field_name . '_wrapper'][$field_name]['#default_value'])) {
                // Decrement usage for replaced file.
                $old_file = file_load($form['image']['#default_value']);
                if ($old_file) {
                  file_usage_delete($old_file, 'layout', 'block', $old_file->fid);
                }
              }
            }
            // Store the filepath separately for use as inline background image.
            $absolute_path = parse_url($GLOBALS['base_url'], PHP_URL_PATH) . '/';
            $fileurl = file_create_url($file->uri);
            $filepath = str_replace($GLOBALS['base_url'] . '/', $absolute_path, $fileurl);
            $form_state['values'][$field_name . '_path'] = $filepath;
            $this->settings[$field_name] = $form_state['values'][$field_name];
            $this->settings[$field_name . '_path'] = $form_state['values'][$field_name . '_path'];
          }
          else {
            // No new file but an old one existed, decrement usage.
            if (!empty($form[$field_name . '_wrapper'][$field_name]['#default_value'])) {
              $old_file = file_load($form[$field_name . '_wrapper'][$field_name]['#default_value']);
              if ($old_file) {
                file_usage_delete($old_file, 'layout', 'block', $old_file->fid);
              }
            }
            $this->settings[$field_name] = NULL;
            $this->settings[$field_name . '_path'] = NULL;
          }

          $this->settings[$field_name . '_field'] = $form_state['values'][$field_name . '_field'];
          break;
      }
    }
  }

  function getFieldApiFields($types = []) {
    static $field_api_info;

    if (empty($field_api_info)) {
      $field_api_info = field_info_fields();
    }

    $fields = [];
    foreach ($field_api_info as $field_name => $field_info) {
      if (in_array($field_info['type'], $types)) {
        $fields[$field_name] = $field_name;
      }
    }

    return $fields;
  }
}
