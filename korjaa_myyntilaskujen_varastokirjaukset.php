<?php

	// otetaan includepath aina rootista
	ini_set("include_path", ini_get("include_path").PATH_SEPARATOR.dirname(__FILE__).PATH_SEPARATOR."/usr/share/pear");
	error_reporting(E_ALL);
	ini_set("display_errors", 1);

	// otetaan tietokanta connect
	require("inc/connect.inc");
	require("inc/functions.inc");

	// Pupeasennuksen root
	$pupe_root_polku = dirname(dirname(__FILE__));

	if (!isset($argv[1]) or !isset($argv[2]) or !isset($argv[3])) {
		echo utf8_encode("VIRHE: pakollisia parametreja puuttu!")."\n";
		exit;
	}

	// Yhti�
	$yhtio = $argv[1];

	// T�st� eteenp�in tapahtumia korjataan
	$accident_date     = $argv[2];
	$accident_date_end = $argv[3];

	$korjaa = FALSE;

	if (isset($argv[4]) and $argv[4] != "") {
		$korjaa = TRUE;
	}

	require("inc/connect.inc");
	require("inc/functions.inc");

	$yhtiorow = hae_yhtion_parametrit($yhtio);
	$kukarow = hae_kukarow('admin', $yhtiorow['yhtio']);

	if (!isset($kukarow)) {
		echo utf8_encode("VIRHE: admin-k�ytt�j�� ei l�ydy!")."\n";
		exit;
	}

	// haetaan halutut varastotaphtumat
	$query  = "	SELECT yhtio, tunnus, tapvm, month(tapvm) KUUKAUSI
				FROM lasku
				WHERE yhtio  = '$kukarow[yhtio]'
				and tila     = 'U'
				and alatila  = 'X'
				and tapvm   >= '$accident_date'
				and tapvm   <= '$accident_date_end'
				and laskunro > 0
				ORDER BY tapvm";
	$result = pupe_query($query);

	if (mysql_num_rows($result) > 0) {

		$eroyht = 0;
		$edkuukausi = 0;

		while ($laskurow = mysql_fetch_assoc($result)) {

			$query  = "	SELECT round(sum(tilausrivi.rivihinta-tilausrivi.kate), 2) varmuutos, group_concat(tilausrivi.tunnus) tunnukset
						FROM tilausrivi
						JOIN tuote ON (tilausrivi.yhtio = tuote.yhtio and tilausrivi.tuoteno = tuote.tuoteno and tuote.ei_saldoa = '')
						WHERE tilausrivi.yhtio     = '$kukarow[yhtio]'
						and tilausrivi.tyyppi      = 'L'
						and tilausrivi.uusiotunnus = {$laskurow['tunnus']}";
			$rivires = pupe_query($query);
			$rivirow = mysql_fetch_assoc($rivires);

			if ($rivirow['tunnukset'] != "") {

				$query  = "	SELECT sum(summa) varmuutos, count(*) varmuma, group_concat(tunnus) varmuutokset
							FROM tiliointi
							WHERE yhtio  = '$kukarow[yhtio]'
							and ltunnus  = $laskurow[tunnus]
							and korjattu = ''
							and tilino   in ('$yhtiorow[varastonmuutos]','$yhtiorow[raaka_ainevarastonmuutos]')";
				$tilires = pupe_query($query);
				$tilirow = mysql_fetch_assoc($tilires);

				$query  = "	SELECT sum(summa) varasto, group_concat(tunnus) varastot
							FROM tiliointi
							WHERE yhtio  = '$kukarow[yhtio]'
							and ltunnus  = $laskurow[tunnus]
							and korjattu = ''
							and tilino   in ('$yhtiorow[varasto]','$yhtiorow[raaka_ainevarasto]')";
				$varares = pupe_query($query);
				$vararow = mysql_fetch_assoc($varares);

				$query  = "	SELECT sum(tapahtuma.hinta * tapahtuma.kpl) * -1 varmuutos
							FROM tapahtuma
							JOIN tuote ON (tapahtuma.yhtio = tuote.yhtio and tapahtuma.tuoteno = tuote.tuoteno and tuote.ei_saldoa = '')
							WHERE tapahtuma.yhtio = '$kukarow[yhtio]'
							and tapahtuma.laji    = 'laskutus'
							and tapahtuma.rivitunnus in ({$rivirow['tunnukset']})";
				$tapares = pupe_query($query);
				$taparow = mysql_fetch_assoc($tapares);

				// Kirjanpito - Tilausrivi
				$ero1 = round($tilirow['varmuutos']-$rivirow["varmuutos"], 2);

				// Tapahtuma - Kirjanpito
				$ero2 = round($tilirow['varmuutos']-$taparow["varmuutos"], 2);

				// Tapahtuma - Tilausrivi
				$ero3 = round($taparow['varmuutos']-$rivirow["varmuutos"], 2);

				if (abs($ero1) > 0.5 or abs($ero2) > 0.5 or abs($ero3) > 0.5) {

					if ($korjaa) {
						// Korjataan tositteet
						if ($tilirow['varmuutokset'] != "") {

							// Tehd��n uusi varastonmuutostili�inti
							$params = array(
								'summa' 		=> round($taparow["varmuutos"], 2),
								'korjattu' 		=> '',
								'korjausaika' 	=> '',
								'laatija' 		=> $kukarow['kuka'],
								'laadittu' 		=> date('Y-m-d H:i:s'),
							);

							$ekamuutos = explode(",", $tilirow['varmuutokset']);

							// Tehd��n vastakirjaus alkuper�iselle varastonmuutostili�innille
							kopioitiliointi($ekamuutos[0], "", $params);

							// Yliviivataan alkuper�iset varastonmuutostili�innit
							$query = "	UPDATE tiliointi
										SET korjattu = '{$kukarow['kuka']}', korjausaika = now()
										WHERE yhtio  = '$kukarow[yhtio]'
										and ltunnus  = $laskurow[tunnus]
										and korjattu = ''
										and tilino   in ('$yhtiorow[varastonmuutos]','$yhtiorow[raaka_ainevarastonmuutos]')
										and tunnus   in ({$tilirow['varmuutokset']})";
							pupe_query($query);
						}

						if ($vararow['varastot'] != "") {

							// Tehd��n uusi varastonmuutostili�inti
							$params = array(
								'summa' 		=> round($taparow["varmuutos"] * -1, 2),
								'korjattu' 		=> '',
								'korjausaika' 	=> '',
								'laatija' 		=> $kukarow['kuka'],
								'laadittu' 		=> date('Y-m-d H:i:s'),
							);

							$ekamuutos = explode(",", $vararow['varastot']);

							// Tehd��n vastakirjaus alkuper�iselle varastonmuutostili�innille
							kopioitiliointi($ekamuutos[0], "", $params);

							// Yliviivataan alkuper�iset varastonmuutostili�innit
							$query = "	UPDATE tiliointi
										SET korjattu = '{$kukarow['kuka']}', korjausaika = now()
										WHERE yhtio  = '$kukarow[yhtio]'
										and ltunnus  = $laskurow[tunnus]
										and korjattu = ''
										and tilino   in ('$yhtiorow[varasto]','$yhtiorow[raaka_ainevarasto]')
										and tunnus   in ({$vararow['varastot']})";
							pupe_query($query);
						}
					}

					$eroyht += $ero2;
				}
			}

			if ($laskurow["KUUKAUSI"] != $edkuukausi and $edkuukausi != 0) {
				if ($eroyht != 0) echo utf8_encode("$yhtiorow[nimi] / $edkuukausi ero yhteens�: $eroyht")."\n";
				flush();
				$eroyht = 0;
			}

			$edkuukausi = $laskurow["KUUKAUSI"];
		}

		if ($eroyht != 0) echo utf8_encode("$yhtiorow[nimi] / $edkuukausi ero yhteens�: $eroyht")."\n";
		echo "\n";
		flush();
	}