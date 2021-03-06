<?php

//Luodaan tarvittavat muuttujat rahtikirja_postitarra_pdf.inc

if ($phpnimi == "rahtikirja_custom.php") {
  $postirow = $osoitelappurow;
  $rakir_row = $osoitelappurow;
}
else {
  $query = "SELECT * from lasku where yhtio = '$kukarow[yhtio]' and tunnus in ($otunnukset) order by tunnus limit 1";
  $tempr = pupe_query($query);
  $postirow = mysql_fetch_array($tempr);
}

// haetaan varaston osoitetiedot, k�ytet��n niit� l�hetystietoina
$query = "SELECT nimi, nimitark, osoite, postino, postitp, maa
          FROM varastopaikat
          WHERE yhtio = '$kukarow[yhtio]'
          AND tunnus  = '$postirow[varasto]'";
$tempr = pupe_query($query);
$postirow_varasto = mysql_fetch_array($tempr);

// jos varastolle on annettu joku osoite, k�ytet��n sit�
if ($postirow_varasto["nimi"] != "") {
  $postirow["yhtio_nimi"]     = $postirow_varasto["nimi"];
  $postirow['yhtio_nimitark']  = $postirow_varasto["nimitark"];
  $postirow["yhtio_osoite"]   = $postirow_varasto["osoite"];
  $postirow["yhtio_postino"]  = $postirow_varasto["postino"];
  $postirow["yhtio_postitp"]  = $postirow_varasto["postitp"];
  $postirow["yhtio_maa"]      = $postirow_varasto["maa"];
}

// jos meill� on printterin takana joku spessu osoitetieto niin k�ytet��n sen tietoja l�hett�j�n tietoina
if ($print["nimi"] != "") {
  $postirow["yhtio_nimi"]    = $print["nimi"];
  $postirow["yhtio_osoite"]  = $print["osoite"];
  $postirow["yhtio_postino"] = $print["postino"];
  $postirow["yhtio_postitp"] = $print["postitp"];
}

if ($postirow['rahtisopimus']=='') $postirow['rahtisopimus'] = "000000";
if ($toitarow['sopimusnro']=='')   $toitarow['sopimusnro']   = "000000";

// postiennakko
if ($rakir_row["jv"] != '' or $mehto['jv'] != '') {
  $postiennakkomaara  = "$yhteensa $postirow[valkoodi]";
  if ($yhtiorow["pankkiiban1"] != '') {
    $postiennakkotilino = "$yhtiorow[pankkiiban1]";
  }
  else {
    $postiennakkotilino = "$yhtiorow[pankkitili1]";
  }
  $postiennakkobic  = "$yhtiorow[pankkiswift1]";
  $postiennakkoviite  = "$viite";
}
else {
  $postiennakkomaara  = "";
  $postiennakkotilino = "";
  $postiennakkobic  = "";
  $postiennakkoviite  = "";
}

// Lis�palvelut
// 1 = Postiennakko
// 3 = Maksaja muu kuin l�hett�j�
// 4 = S�rkyv�
// 6 = Lauantaijakelu

$x = $x_tekstit = $x_tekstit_koko = array();

// Postiennakko
if ($yhteensa != '') {
  $x[] = '1';
  $x_tekstit[1] = 'POSTIENNAKKO';
  $x_tekstit_koko[1] = 'POSTIENNAKKO';
}

// Maksaja muu kuin l�hett�j�
if ($rakir_row['merahti'] != 'K') {
  $x[] = '3';
  $x_tekstit[3] = 'MAKSAJA MUU';
  $x_tekstit_koko[3] = 'MAKSAJA MUU KUIN L�HETT�J�';
}

// S�rkyv�
if ($toitarow['erilliskasiteltavakulu'] != 0) {
  $x[] = '4';
  $x_tekstit[4] = 'S�RKYV�';
  $x_tekstit_koko[4] = 'S�RKYV�';
}

// Lauantaijakelu
if ($toitarow['lauantai'] != '') {
  $x[] = '6';
  $x_tekstit[6] = 'LAUANTAIJAKELU';
  $x_tekstit_koko[6] = 'LAUANTAIJAKELU';
}

// Asetetaan rahtikirjan tiedot tulostettavaan muotoon
// tuoteviiva1 = postin palvelukoodi, joka tulostetaan viivakoodina rahtikirjaan
// $keltainen  = toimitustavan nimi, joka tulostetaan ensimm�iselle riville
// $kuljetus   = toimitustavan nimen lis�tieto, joka tulostetaan toiselle riville
// $keku       = toimitustavan numero, joka tulostetaan isolla fontilla nimen viereen
switch ($toitarow['virallinen_selite']) {
  case 'Pikkupaketti':
    $tuoteviiva1 = '2W2461';
    $keltainen   = 'Pikkupaketti';
    $kuljetus    = '';
    $keku        = 'P';

    break;
  case 'Postipaketti':
    $tuoteviiva1 = '2W2103';
    $keltainen   = 'Postipaketti';
    $kuljetus    = '';
    $keku        = '16';

    break;
  case 'Kotipaketti':
    $tuoteviiva1 = '2W2104';
    $keltainen   = 'Kotipaketti';
    $kuljetus    = '';
    $keku        = '21';

    break;
  case 'Express-paketti':
    $tuoteviiva1 = '2W2102';
    $keltainen   = 'Express-paketti';
    $kuljetus    = '';
    $keku        = '14';

    break;
  case 'Express-paketti Aamuksi 09':
    $tuoteviiva1 = '2W2101';
    $keltainen   = 'Express-paketti';
    $kuljetus    = 'Aamuksi';
    $keku        = '9';

    break;
  case 'Express-paketti Samana P�iv�n� 00':
    $tuoteviiva1 = '2W2124';
    $keltainen   = 'Express-paketti';
    $kuljetus    = 'Samana P�iv�n�';
    $keku        = '00';

    break;
  case 'Express-rahti':
    $tuoteviiva1 = '2W2144';
    $keltainen   = 'Express-rahti';
    $kuljetus    = '';
    $keku        = '14';

    break;
  case 'Express-rahti Aamuksi 09':
    $tuoteviiva1 = '2W2143';
    $keltainen   = 'Express-rahti';
    $kuljetus    = 'Aamuksi';
    $keku        = '9';

    break;
  case 'Express-rahti Illaksi 21':
    $tuoteviiva1 = '2W2145';
    $keltainen   = 'Express-rahti';
    $kuljetus    = 'Illaksi';
    $keku        = '21';

    break;
  case 'Express-rahti Samana P�iv�n� 00':
    $tuoteviiva1 = '2W2142';
    $keltainen   = 'Express-rahti';
    $kuljetus    = 'Samana P�iv�n�';
    $keku        = '00';

    break;
  case 'Priority':
    $tuoteviiva1 = '2W2015';
    $keltainen   = 'Priority';
    $kuljetus    = '';
    $keku        = '';

    break;
  default:
    // Oletuksena: Posti Express-paketti 14
    $tuoteviiva1 = '2W2102';
    $keltainen   = 'Express-paketti';
    $kuljetus    = '';
    $keku        = '14';

    break;
}

if (count($vakit) > 0) {
  // T�m� tyhjennet��n kun tulostetaan VAK/ADR rahtikirja
  $keku = "";
}

if ($rahdinmaksaja == 'L�hett�j�') {
  $rahdinmaksaja    = "";
  $rahdinmaksajan_nr   = "";
}
elseif (trim($rakir_row['rahtisopimus']) != "") {
  $rahdinmaksaja    = "Maksaja muu kuin l�hett�j�. ";
  $rahdinmaksajan_nr   = $rakir_row['rahtisopimus'];
}
else {
  $rahdinmaksaja    = "Maksaja muu kuin l�hett�j�. ";
  $rahdinmaksajan_nr   = "";
}

// Ulkomaan l�hetykiss� tarvitaan my�s nelinumeroinen, asiakaskohtainen tunnushallintanumero
if (strpos($toitarow['sopimusnro'], "@") !== FALSE) {
  list($toitarow['sopimusnro'], $toitarow['tunhalnro']) = explode("@", $toitarow['sopimusnro']);
}

$toitarow['sopimusnro'] = sprintf("%06s", $toitarow['sopimusnro']);  // sopimunumeron tulee olla kuus pitk�
$sopnro  = $toitarow['sopimusnro']; // k�ytet��n edi sanomassa
