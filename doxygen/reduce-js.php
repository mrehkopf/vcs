<?php

/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 * Modifies (reduces) Doxygen-generated JavaScript files to make them compliant with
 * VCS's custom Doxygen theme.
 * 
 */

echo "Reducing the output's JS...\n";

$fileReducers = [
    "./html/menudata.js"=>[
        "singly_capitalize",
    ],
    "./html/navtreedata.js"=>[
        "singly_capitalize",
    ],
];

$filenames = array_keys($fileReducers);

foreach ($filenames as $filename)
{
    $reducedContents = file_get_contents($filename);

    foreach ($fileReducers[$filename] as $reducerFn)
    {
        $reducedContents = $reducerFn($reducedContents);
    }

    file_put_contents($filename, $reducedContents);
}

function singly_capitalize(string $js) : string
{
    $replacements = [
        '"Main Page"'=>'"Main page"',
        '"Data Fields"'=>'"Data fields"',
        '"Data Structures"'=>'"Data structures"',
        '"Data Structure Index"'=>'"Data structure index"',
        '"File List"'=>'"File list"',
        '"VCS Developer Documentation"'=>'"Main page"',
    ];

    return strtr($js, $replacements);
}
