<?php

/**
 * @file
 * This file contains all the utility functions for FITS commands - used in drush call to create batch jobs.
 */

/**
 * Create/Update a datastream with the given parameters.
 *
 * @param AbstractObject $object
 *   The object to update.
 * @param string $filename
 *   The absolute path to the file to use as content for the datastream.
 * @param string $dsid
 *   The datastream ID.
 * @param string $label
 *   The datastream label.
 * @param string $mimetype
 *   The datastream mimetype, if none given it will be guessed from the file
 *   extension.
 * @param string $control_group
 *   The datastream control group.
 *
 * @return bool
 *   TRUE if successful, FALSE otherwise.
 */
function islandora_fits_update_datastream(AbstractObject $object, $filename, $dsid, $label = NULL, $mimetype = NULL, $control_group = 'M', $copy = TRUE) {
  if (!isset($mimetype)) {
    $mime_detector = new MimeDetect();
    $mimetype = $mime_detector->getMimetype($file);
  }
  $label = isset($label) ? $label : $dsid;

  if (!isset($object[$dsid])) {
    $ds = $object->constructDatastream($dsid, $control_group);
    $ds->mimetype = $mimetype;
    $ds->label = $label;
    $ds->setContentFromFile($filename, $copy);
    $object->ingestDatastream($ds);
  }
  else {
    $ds = $object[$dsid];
    $ds->setContentFromFile($filename, $copy);
    if ($ds->label != $label) {
      $ds->label = $label;
    }
    if ($ds->mimetype != $mimetype) {
      $ds->mimetype = $mimetype;
    }
  }
  return TRUE;
}
