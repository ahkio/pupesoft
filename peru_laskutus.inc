<?php

if (!function_exists("peru_laskutus")) {
  function peru_laskutus($laskunro, $params = array()) {
    global $kukarow;

    $peru_numeroon    = isset($params["peru_numeroon"])    ? $params["peru_numeroon"]    : $laskunro;
    $hard_delete      = isset($params["hard_delete"])      ? $params["hard_delete"]      : false;
    $poista_liittyvat = isset($params["poista_liittyvat"]) ? $params["poista_liittyvat"] : true;

    // L�ytyyk� U-laskua
    $query = "SELECT *
              FROM lasku
              WHERE yhtio = '{$kukarow["yhtio"]}'
              AND laskunro >= '{$laskunro}'
              AND laskunro <= '{$peru_numeroon}'
              AND tila = 'U'
              AND alatila = 'X'";
    $ulaskures = pupe_query($query);

    while ($ulaskurow = mysql_fetch_array($ulaskures)) {
      if ($hard_delete) {
        // Poistetaan tili�innit
        $query = "DELETE FROM tiliointi
                  WHERE yhtio = '{$kukarow["yhtio"]}'
                  AND ltunnus = '{$ulaskurow["tunnus"]}'";
      }
      else {
        // Merkit��n tili�innit korjatuksi
        $query = "UPDATE tiliointi SET
                  korjattu    = '{$kukarow["kuka"]}',
                  korjausaika = now()
                  WHERE yhtio = '{$kukarow["yhtio"]}'
                  AND ltunnus = '{$ulaskurow["tunnus"]}'";
      }

      pupe_query($query);

      if ($hard_delete) {
        // Deletoidaan U-lasku
        $query = "DELETE FROM lasku
                  WHERE yhtio	= '{$kukarow["yhtio"]}'
                  AND tila = 'U'
                  AND alatila = 'X'
                  AND tunnus = '{$ulaskurow["tunnus"]}'";
      }
      else {
        // Merkataan lasku poistetuksi
        $query = "UPDATE lasku SET
                  tila      = 'D',
                  alatila   = tila,
                  muuttaja  = '{$kukarow["kuka"]}',
                  muutospvm = now()
                  WHERE yhtio	= '{$kukarow["yhtio"]}'
                  AND tila = 'U'
                  AND alatila = 'X'
                  AND tunnus = '{$ulaskurow["tunnus"]}'";
      }

      pupe_query($query);

      // L�ytyyk� L-laskua
      $query = "SELECT *
                FROM lasku
                WHERE yhtio = '{$kukarow["yhtio"]}'
                AND tila = 'L'
                AND alatila = 'X'
                AND laskunro = '{$ulaskurow["laskunro"]}'";
      pupe_query($query);

      while ($laskurow = mysql_fetch_array($llaskures)) {
        if ($laskurow["vienti"] != "") {
          $alatila = "E";
        }
        else {
          $alatila = "D";
        }

        if ($poista_liittyvat) {
          if ($hard_delete) {
            // Poistetaan tilausotsikko
            $query = "DELETE FROM lasku
                      WHERE yhtio	= '{$kukarow["yhtio"]}'
                      AND tila = 'L'
                      AND alatila	= 'X'
                      AND tunnus = '{$laskurow["tunnus"]}'";
          }
          else {
            // Merkit��n tilausotsikko poistetuksi
            $query = "UPDATE lasku SET
                      tila      = 'D',
                      alatila   = tila,
                      muuttaja  = '{$kukarow["kuka"]}',
                      muutospvm = now()
                      WHERE yhtio	= '{$kukarow["yhtio"]}'
                      AND tila = 'L'
                      AND alatila	= 'X'
                      AND tunnus = '{$laskurow["tunnus"]}'";
          }
        }
        else {
          // P�ivitet��n tilausotsikko takaisin tilaan toimitettu / vientitiedot sy�tetty
          $query = "UPDATE lasku SET
                    alatila	              = '{$alatila}',
                    laskuttaja            = '',
                    laskutettu            = '0000-00-00 00:00:00',
                    tapvm                 = '0000-00-00',
                    erpcm                 = '0000-00-00',
                    laskunro              = 0,
                    summa                 = 0,
                    kate                  = 0,
                    arvo                  = 0,
                    viite                 = '',
                    factoringsiirtonumero = 0
                    WHERE yhtio	= '{$kukarow["yhtio"]}'
                    AND tila = 'L'
                    AND alatila	= 'X'
                    AND tunnus = '{$laskurow["tunnus"]}'";
        }

        pupe_query($query);

        // Hoidetaan tapahtumat ja saldot
        $query = "SELECT *
                  FROM tilausrivi
                  WHERE yhtio	= '{$kukarow["yhtio"]}'
                  AND otunnus	= '{$laskurow["tunnus"]}'";
        $res = pupe_query($query);

        while ($rivirow = mysql_fetch_array($res)) {
          //Poistetaan tapahtmat
          $query = "DELETE from tapahtuma
                    WHERE yhtio = '{$kukarow["yhtio"]}'
                    AND rivitunnus = '{$rivirow["tunnus"]}'";
          pupe_query($query);

          // P�ivitet��n saldo tilausrivin osoittamalla paikalla
          $query = "UPDATE tuotepaikat SET
                    saldo = saldo + {$rivirow["kpl"]}
                    WHERE yhtio = '{$kukarow["yhtio"]}'
                    AND tuoteno = '{$rivirow["tuoteno"]}'
                    AND hyllyalue = '{$rivirow["hyllyalue"]}'
                    AND hyllynro = '{$rivirow["hyllynro"]}'
                    AND hyllytaso = '{$rivirow["hyllytaso"]}'
                    AND hyllyvali = '{$rivirow["hyllyvali"]}'";
          pupe_query($query);

          if (mysql_affected_rows() == 0) {
            // P�ivitet��n saldo tuotteen oletuspaikalla jos tilausrivill� ei ollut sopivaa paikkaa
            $query = "UPDATE tuotepaikat
                      SET saldo = saldo + {$rivirow["kpl"]}
                      WHERE yhtio = '{$kukarow["yhtio"]}'
                      AND tuoteno	= '{$rivirow["tuoteno"]}'
                      AND oletus != ''
                      LIMIT 1";
            pupe_query($query);
          }
        }

        if ($poista_liittyvat) {
          if ($hard_delete) {
            // Poistetaan tilausrivit
            $query = "DELETE FROM tilausrivi
                      WHERE yhtio = '{$kukarow["yhtio"]}'
                      AND otunnus = '{$laskurow["tunnus"]}'";
            pupe_query($query);
          }
          else {
            // Merkit��n tilausrivit poistetuksi
            $query = "UPDATE tilausrivi SET
                      tyyppi = 'D'
                      WHERE yhtio = '{$kukarow["yhtio"]}'
                      AND otunnus = '{$laskurow["tunnus"]}'";
            pupe_query($query);
          }
        }
        else {
          // P�ivitet��n tilausrivit
          $query = "UPDATE tilausrivi SET
                    varattu        = kpl,
                    kpl   	       = 0,
                    laskutettu     = '',
                    laskutettuaika = '0000-00-00',
                    uusiotunnus	   = 0,
                    rivihinta			 = 0,
                    kate				   = 0
                    WHERE yhtio = '{$kukarow["yhtio"]}'
                    AND otunnus = '{$laskurow["tunnus"]}'";
          pupe_query($query);

          $query = "UPDATE tilausrivi SET
                    kommentti = ''
                    WHERE yhtio = '{$kukarow["yhtio"]}'
                    AND otunnus = '{$laskurow["tunnus"]}'
                    AND kommentti LIKE 'Tilaus%'";
          pupe_query($query);

          $query = "DELETE FROM tilausrivi
                    WHERE yhtio = '{$kukarow["yhtio"]}'
                    AND otunnus = '{$laskurow["tunnus"]}'
                    AND tuoteno = '{$yhtiorow["rahti_tuotenumero"]}'";
          pupe_query($query);
        }
      }
    }
  }
}