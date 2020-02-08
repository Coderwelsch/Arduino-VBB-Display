<?php
	// header
	header('Content-type:application/json;charset=utf-8');

    $rawApiData = file_get_contents("https://2.db.transport.rest/journeys?from=736150&to=736100&departure=in%206%20minutes&results=5");

    if (!$rawApiData) {
    	die("API ERROR");
    }

    $jsonData = json_decode($rawApiData);
    $responseData = array();

    foreach ($jsonData as $entry) {
    	if (count($entry->legs)) {
    		$legs = $entry->legs[0];

			// filter journeys, where departure time 
			// is at least 3 mins in future
			$timeDiff = strtotime($legs->departure) - time();

    		if ($timeDiff < 180 ||
                empty($legs->line->name) > 0 ||
                empty($legs->direction)) {
                continue;
    		}
    		
    		$object = new stdClass();
    		$object->t = $timeDiff;
    		$object->n = convertCharactersForESP32($legs->line->name);
    		$object->d = convertCharactersForESP32($legs->direction);

    		$responseData[] = $object;
    	}
    }

    function convertCharactersForESP32 ($string) {
        $replaceDefs = array(
            "ü" => "ue",
            "ä" => "ae",
            "ö" => "oe",
            "Ü" => "Ue",
            "Ä" => "Ae",
            "Ö" => "Oe",
            "ß" => "ss"
        );

        $string = strtr($string, $replaceDefs);

        return substr($string, 0, 10);
    }

    usort($&, function ($a, $b) {
		return $a->t - $b->t;
    });

    echo json_encode(array_slice($responseData, 0, 2));