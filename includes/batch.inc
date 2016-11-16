<?php

/**
 * @file
 * This file contains all the batch operations for paged content.
 */

/**
 * Sets up a batch operation.
 *
 * @param array $operands
 *   All the pages to operate on.
 * @param array $context
 *   The context of this batch operation.
 * @param array $message
 *   Values are used as parameters to t() and drupal_set_message() that will be
 *   displayed when the batch is finished. Placeholders include, but are not
 *   limited to: @successful, @failed, @total.
 *
 * @return mixed
 *   The current operand the operation will process.
 */
function islandora_fits_start_batch_operation(array $operands, &$context, array $message = array()) {
  if (!isset($context['sandbox']['progress'])) {
    $results =& $context['results'];
    if (empty($results)) {
      // "files" will be deleted upon failure or success of the batch.
      // "successful" an array of boolean values indicating operation success.
      // "messages" array of messages to display on finishing the batch
      // values are used as parameters to t() and drupal_set_message().
      $results = array(
        'files' => array(),
        'successful' => array(),
        'messages' => array(),
      );
    }
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['total'] = count($operands);
    $context['results']['messages'][] = $message;
  }
  $operand_index = $context['sandbox']['progress'];
  return isset($operands[$operand_index]) ? $operands[$operand_index] : FALSE;
}

/**
 * Ends the batch operation.
 *
 * @param array $context
 *   The context of this batch operation.
 * @param int $count
 *   The number of steps taken in the operation, typically 1.
 * @param array $message
 *   Values are used as parameters to t() and drupal_set_message() that will be
 *   displayed when the batch is finished.
 *
 * @see islandora_fits_start_batch_operation()
 *
 * @return bool
 *   TRUE if the batch operation is complete, FALSE otherwise.
 */
function islandora_fits_end_batch_operation(&$context, $count = 1, $message = array()) {
  $context['sandbox']['progress'] += $count;
  if ($context['sandbox']['progress'] != $context['sandbox']['total']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['total'];
    return FALSE;
  }
  $context['results']['messages'][] = $message;
  return TRUE;
}

/**
 * The batch 'finished' callback.
 */
function islandora_fits_batch_finished($success, $results, $operations) {
  if ($success) {
    // Delete any lingering files.
    if (isset($results['files'])) {
      foreach ($results['files'] as $file) {
        error_log('DELETE ' . $file);
        if (file_exists($file)) {
          file_unmanaged_delete($file);
        }
      }
    }
    // Build generic place holders for the message.
    if (isset($results['successful'])) {
      $total = count($results['successful']);
      $successful = count(array_filter($results['successful']));
      $failed = $total - $successful;
      $place_holders = array(
        '@successful' => $successful,
        '@failed' => $failed,
        '@total' => $total,
      );
      // Display all messages/errors/warnings.
      $messages = array_filter($results['messages']);
      foreach ($messages as $message) {
        $vars = isset($message[1]) ? array_merge($message[1], $place_holders) : $place_holders;
        $status = isset($message[2]) ? $message[2] : 'status';
        $message = format_string($message[0], $vars);
        drupal_set_message(check_plain($message), $status);
      }
    }
  }
  else {
    // Generic Error Message.
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array(
                 '%error_operation' => $error_operation[0],
                 '@arguments' => print_r($error_operation[1], TRUE)));
    drupal_set_message($message, 'error');
  }
}

/**
 * Gets the batch definition for creating FITS datastream.
 *
 * @param AbstractObject $paged_content
 *   The paged content the pages belong to.
 * @param array $pages
 *   All the pages to process.
 * @param array $options
 *   Options to pass into tesseract/gimp.
 *
 * @return array
 *   The batch definition.
 */
function islandora_fits_create_batch(AbstractObject $paged_content, array $pages, array $options) {
  $return = array(
    'operations' => array(
      array('islandora_fits_generate_techmd_batch_operation', array(
        $pages, $options)),
    ),
    'finished' => 'islandora_fits_batch_finished',
    'title' => t('Performing FITS on @label ...', array('@label' => $paged_content->label)),
    'init_message' => t('Preparing to perform FITS on @label ...', array('@label' => $paged_content->label)),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'islandora_fits') . '/includes/batch.inc',
  );
  return $return;
}

/**
 * Batch operation for creating the FITS datastream.
 *
 * @param array $pages
 *   All the pages to run FITS on.
 * @param array $options
 *   Options to pass into tesseract/gimp.
 * @param array $context
 *   The context of this batch operation.
 */
function islandora_fits_generate_techmd_batch_operation(array $pages, array $options, &$context) {
  module_load_include('module', 'islandora_fits');
  module_load_include('inc', 'islandora_fits', 'includes/utilities');
  module_load_include('inc', 'islandora_fits', 'includes/derivatives');

  $page = islandora_fits_start_batch_operation($pages, $context);
  if ($page) {
    $dsid = 'TECHMD';
    $page = islandora_object_load($page);
    $result = islandora_fits_create_techmd($page, $options['force'], $options);

    // Add this orphan file reference for cleanup during the callback_batch_finished.
    $context['results']['files'][] = 'temporary://' . str_replace(":", "_", $page) . '_' . $dsid . '.xml';

    islandora_fits_end_batch_operation($context, 1, array(
      'Derived @derived of @set_size TECHMD datastreams.',
      array(
        '@set_size' => count($context['results']['successful']),
        '@derived' => count(array_filter($context['results']['successful'])),
      )));
    $context['message'] = t('Generating FITS datastream on page @page of @total ...', array(
                            '@page' => $context['sandbox']['progress'],
                            '@total' => $context['sandbox']['total']));
  }
}
