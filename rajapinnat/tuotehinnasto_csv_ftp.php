<?php

/*
 * Tehd��n Pupesoftin hinnastodatasta CSV ja l�hetet��n se FTP:ll�
*/

//* T�m� skripti k�ytt�� slave-tietokantapalvelinta *//
$useslave = 1;

// Kutsutaanko CLI:st�
if (php_sapi_name() != 'cli') {
  die ("T�t� scripti� voi ajaa vain komentorivilt�!");
}

$pupe_root_polku = dirname(dirname(__FILE__));

if (!isset($argv[1]) or $argv[1] == '') {
  die("Yhti� on annettava!!");
}

ini_set("memory_limit", "5G");

// Otetaan includepath aina rootista
ini_set("include_path", ini_get("include_path").PATH_SEPARATOR.$pupe_root_polku);

require 'inc/connect.inc';
require 'inc/functions.inc';

// Logitetaan ajo
cron_log();

// Yhti�
$yhtio = mysql_real_escape_string($argv[1]);

$yhtiorow = hae_yhtion_parametrit($yhtio);

$ajohetki = date("Y-m-d_His");

// Tallennetaan rivit tiedostoon
$filepath = "/tmp/tuotehinnasto_{$yhtio}_$ajohetki.csv";

if (!$fp = fopen($filepath, 'w+')) {
  die("Tiedoston avaus ep�onnistui: $filepath\n");
}

// Otsikkotieto
$header = "tuoteno;minkpl;maxkpl;hinta;alkupvm;loppupvm;laji;maa;valkoodi;selite;myyntihinta\n";
fwrite($fp, $header);

// Haetaan hinnasto-taulun sis�lt�
$query = "SELECT hinnasto.*, tuote.myyntihinta
          FROM hinnasto join tuote on (tuote.yhtio = hinnasto.yhtio and tuote.tuoteno = hinnasto.tuoteno)
          WHERE hinnasto.yhtio  = '{$yhtiorow['yhtio']}'";
$res = pupe_query($query);

// Kerrotaan montako rivi� k�sitell��n
$rows = mysql_num_rows($res);

echo date("d.m.Y @ G:i:s") . ": Hinnastorivej� {$rows} kappaletta.\n";

while ($row = mysql_fetch_assoc($res)) {

  $rivi  = "{$row['tuoteno']};";
  $rivi  .= "{$row['minkpl']};";
  $rivi  .= "{$row['maxkpl']};";
  $rivi  .= "{$row['hinta']};";
  $rivi  .= "{$row['alkupvm']};";
  $rivi  .= "{$row['loppupvm']};";
  $rivi  .= "{$row['laji']};";
  $rivi  .= "{$row['maa']};";
  $rivi  .= "{$row['valkoodi']};";
  $rivi  .= "{$row['selite']};";
  $rivi  .= "{$row['myyntihinta']};";
  $rivi .= "\n";

  fwrite($fp, $rivi);
}

fclose($fp);

// Tehd��n FTP-siirto
if (!empty($hinnastocsv_ftphost)) {
  $ftphost = $hinnastocsv_ftphost;
  $ftpuser = $hinnastocsv_ftpuser;
  $ftppass = $hinnastocsv_ftppass;
  $ftppath = $hinnastocsv_ftppath;
  $ftpfile = $filepath;
  require "inc/ftp-send.inc";
}

echo date("d.m.Y @ G:i:s") . ": Hinnasto CSV valmis.\n\n";
