<?php

/**
 * @file
 * Drush commands for paged content.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_fits_drush_command() {
  return array(
    'create-techmd' => array(
      'aliases' => array('fitsct'),
      'description' => dt('Generates the TECHMD datastreams given the options.'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'drupal dependencies' => array(
        'islandora_paged_content',
      ),
      'arguments' => array(
        'pid' => 'The PID value for the object.',
      ),
      'options' => array(
        'force' => 'Force regeneration of TECHMD.',
      ),
      'examples' => array(
        'Trigger TECHMD derivatives on islandora:372 and forcing regeneration of datastream' => 'drush -u 1 fitsct islandora:372 --force=1 --uri=http://site.library.pitt.edu/',
        'Trigger TECGMD derivatives on islandora:372' => 'drush -u 1 fitsct islandora:372 --uri=http://site.library.pitt.edu/',
      ),
    ),
    'create-techmd-byfile' => array(
      'aliases' => array('fitsctf'),
      'description' => dt('Generates the TECHMD datastreams given the options - objects based on a file.'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'drupal dependencies' => array(
        'islandora_paged_content',
      ),
      'options' => array(
        'force' => 'Force regeneration of TECHMD.',
        'process_pids_file' => 'Optional file containing PID values on each line to process.',
        'processed_pids_file' => 'File to save PID values as they are processed.',
      ),
      'examples' => array(
        'Trigger TECHMD derivatives on a file of objects and forcing regeneration of datastream' => 'drush -u 1 fitsctf --force=1 --uri=http://site.library.pitt.edu/',
        'Trigger TECGMD derivatives on a file of objects' => 'drush -u 1 fitsctf --process_pids_file=/home/bgilling/pids.txt --processed_pids_file=/home/bgilling/done_pids.txt --uri=http://site.library.pitt.edu/',
      ),
    ),
  );
}

/**
 * Will generate fits datastreams.
 *
 * @param string $pid
 *   The PID for the book or page object for which the derivatives will be generated.
 *
 * Options are not defined in the function declaration, but can be derived using drush_get_option() calls
 * @param array $options
 *   The options for tesseract/gimp.
 */
function drush_islandora_fits_create_techmd($pid) {
  $batch =& batch_get();

  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  module_load_include('inc', 'islandora_fits', 'includes/batch');
  module_load_include('inc', 'islandora_upitt', 'includes/drush_helpers');
  $time_start = microtime_float();

  $islandora_object = islandora_object_load($pid);
  if (!is_object($islandora_object) || (get_class($islandora_object) <> 'IslandoraFedoraObject')){
    drush_set_error(dt('Could not load object for PID:').$pid);
    return false;
  }
  else {
    $models = $islandora_object->models;
    if (array_search('islandora:pageCModel', $models) === FALSE) {
      $pages = array_keys(islandora_paged_content_get_pages($islandora_object));
    }
    else {
      $pages = array($pid => $pid);
    }
    $page_count = count($pages);
    $force = drush_get_option('force', 0);
    $options = array(
      'language' => 'eng',
      'preprocess' => FALSE,
      'force' => $force,
      'source_dsid' => 'OBJ',
      'destination_dsid' => 'TECHMD',
      'weight' => '0'
    );

    $batch = islandora_fits_create_batch($islandora_object, $pages, $options);
    batch_set($batch);
    $batch =& batch_get();
 
    //Because we are doing this on the back-end, we set progressive to false.
    $batch['progressive'] = FALSE;

    drush_backend_batch_process();
  }
  $time_end = microtime_float();
  $time = $time_end - $time_start;
  error_log(number_format($page_count) . ' pages processed in ' . $time);
  if ($page_count > 0) {
    error_log($time / $page_count . ' seconds per page');
  }
  return TRUE;
}

/**
 * Will generate fits datastreams for objects that are referenced in a "process_pids" file that 
 * contains a single PID value on each line.  The "processed_pids" file is used to store the pids
 * that were already processed - and is used to determine whether or not a pid listed in the "process_pids" 
 * file should be processed.  If there are any errors during the processing of an object, the pid values
 * will be stored in a file that is similar to the "processed_pids" filename, but with "_errors" added before
 * the file extension (eg: "processed.txt" would result in a "processed_errors.txt" file).
 *
 * @param string $pid
 *   The PID for the book or page object for which the derivatives will be generated.
 *
 * Options are not defined in the function declaration, but can be derived using drush_get_option() calls
 * @param array $options
 *   The options for tesseract/gimp.
 */
function drush_islandora_fits_create_techmd_byfile() {
  $batch =& batch_get();

  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  module_load_include('inc', 'islandora_fits', 'includes/batch');
  module_load_include('inc', 'islandora_paged_content', 'includes/helpers');

  $processed_pids_file = drush_get_option('processed_pids_file', '');
  $process_pids_file = drush_get_option('process_pids_file', '');

error_log($process_pids_file);
error_log($processed_pids_file);

  if ($process_pids_file && $processed_pids_file && file_exists($process_pids_file)) {
    $file = file($process_pids_file);
    $done_file = (file_exists($processed_pids_file) ? file($processed_pids_file) : array());

    // Get only the PID values that have not been done yet.
    $pids = array_diff($file, $done_file);
  }
  foreach ($pids as $pid) {
    $pid = trim($pid);
    if ($pid) {
      error_log('create_techmd_byfile for ' . $pid . "\n---------------------------------------------------");
      $gen_fits = drush_islandora_fits_create_techmd($pid);

      if ($processed_pids_file && $gen_fits) {
        file_put_contents($processed_pids_file, $pid . "\n", FILE_APPEND);
      } else {
        $errors_file = str_replace('.', '_errors.', $processed_pids_file);
        $errors_file = (strstr($errors_file, "_errors.")) ? $errors_file : $errors_file . '_errors';
        $pid_and_errors = array($pid);
        if (!$gen_fits) { $pid_and_errors[] = 'generate_ocr_datastreams'; }
        file_put_contents($errors_file, implode(",", $pid_and_errors) . "\n", FILE_APPEND);
      }
    }
  }
  return TRUE;
}
