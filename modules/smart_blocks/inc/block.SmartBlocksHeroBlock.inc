<?php

/**
 *  This class allows us to create hero blocks with image backgrounds.
 */
class SmartBlocksHeroBlock extends SmartBlocksBase {

  public $is_hero_block = TRUE;

  /**
   * {@inheritdoc}
   */
  function __construct($plugin_name, array $data) {
    $this->fields = [
      'content' => [
        'type' => 'text_format',
        'label' => t('Content'),
      ],
      'image' => [
        'type' => 'image',
        'label' => t('Image'),
      ],
    ];

    $this->default_admin_title = t('Smart hero block');

    parent::__construct($plugin_name, $data);
  }

  /**
   * Returns the rendered content of this block.
   *
   * If there is no content, a non-breaking space is returned to ensure the
   * block is still displayed, with only the background image applied.
   *
   * @return string
   */
  function getContent() {
    if (!empty($this->settings['content'])) {
      return check_markup(token_replace($this->settings['content']), $this->settings['content_format'], '', TRUE);
    }

    elseif (!empty($this->settings['title']) || !empty($this->settings['image'])) {
      return '&nbsp;';
    }
  }
}
