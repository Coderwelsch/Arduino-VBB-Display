<?php

// set header for json response / mime type
header('Content-type:application/json;charset=utf-8');

// get params
$FROM_PARAM = $_GET["from"];
$TO_PARAM = $_GET["to"];
$LENGTH_PARAM = $_GET["length"];

$REPLACE_UMLAUTS_DEFS = array(
    "ü" => "ue",
    "ä" => "ae",
    "ö" => "oe",
    "Ü" => "Ue",
    "Ä" => "Ae",
    "Ö" => "Oe",
    "ß" => "ss"
);

// die when a param wasn’t set
if (!isset($FROM_PARAM) || !isset($TO_PARAM) || !isset($LENGTH_PARAM)) {
    http_response_code(400);
    die("MISSING GET PARAM. PARAMS:\nFROM: $FROM_PARAM\nTO: $TO_PARAM\nLENGTH: $LENGTH_PARAM");
}

$url =
    "https://2.db.transport.rest/journeys?" .
    "from=$FROM_PARAM&" .
    "to=$TO_PARAM&" .
    "departure=in%206%20minutes&" .
    "results=$LENGTH_PARAM";

// api response from db.transport.rest
$rawApiData = file_get_contents($url);

if (!$rawApiData) {
    http_response_code(500);
    die("API ERROR");
}

// decode fetched json data
$jsonData = json_decode($rawApiData);

if (!$jsonData) {
    die("JSON ERROR");
}

// creates an empty array for
// further data cleanup
$responseData = array();

foreach ($jsonData as $entry) {
    // sometimes it happens that a ride
    // don’t has any trams (or «legs»)
    // here we are checking just for the first tram
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

        // creates new object for response data
        $object = new stdClass();
        $object->t = $timeDiff;

        // i have to do that, because the arduino doesn’t likes
        // any kind of umlauts and ß …
        $object->n = convertCharactersForESP32($legs->line->name);
        $object->d = convertCharactersForESP32($legs->direction);

        // saves data to the response object
        $responseData[] = $object;
    }
}


function convertCharactersForESP32($string) {
    global $REPLACE_UMLAUTS_DEFS;

    $string = strtr($string, $REPLACE_UMLAUTS_DEFS);
    return substr($string, 0, 10);
}

// sorts the cleaned trams by remaining time (or «t»)
usort($responseData, function ($a, $b) {
    return $a->t - $b->t;
});

// returning response data
echo json_encode(array_slice($responseData, 0, intval($LENGTH_PARAM)));