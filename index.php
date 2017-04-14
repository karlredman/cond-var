<?php

//---------------------------------------------------
// Require the indexer class
//---------------------------------------------------
require_once('../lophtyindexerfiles/lophty_directory_indexer.php');
//---------------------------------------------------
// Get a handle on server vars
//---------------------------------------------------
(!empty($_SERVER)) ? $arrHTTPserverVars = $_SERVER : $arrHTTPserverVars = $HTTP_SERVER_VARS;
//---------------------------------------------------
// Create an instance of the class
//---------------------------------------------------
$classinstLophtyDirectoryIndexer = new lophty_directory_indexer($arrHTTPserverVars['REQUEST_URI'], $arrHTTPserverVars['SERVER_NAME'], $arrHTTPserverVars['SCRIPT_FILENAME']);




//---------------------------------------------------
// Recipe : index current dir.
//---------------------------------------------------
$strListingHTMLstring = $classinstLophtyDirectoryIndexer->indexDirReturnSortedListHTMLChunk();
$classinstLophtyDirectoryIndexer->printIndexerPage($strListingHTMLstring);

?>

