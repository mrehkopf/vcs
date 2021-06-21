<?php

/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 * Modifies Doxygen-generated HTML files to make them compliant with VCS's custom
 * Doxygen theme.
 * 
 */

$htmlFiles = glob("./html/*.html");

foreach ($htmlFiles as $filename)
{
    $strippedContents = file_get_contents($filename);

    $strippedContents = remove_non_breaking_spaces($strippedContents);
    $strippedContents = remove_unnecessary_spaces($strippedContents);
    $strippedContents = simplify_enum_declarations($strippedContents);

    file_put_contents($filename, $strippedContents);
}

echo "Stripped the output\n";

function remove_non_breaking_spaces(string $string) : string
{
    $replacements = [
        "&nbsp;"=>" ",
        "&#160;"=>" ",
    ];

    $stripped = strtr($string, $replacements);

    return $stripped;
}

function remove_unnecessary_spaces(string $string) : string
{
    $simplified = str_replace('class="paramtype">void <', 'class="paramtype">void<', $string);

    return $simplified;
}

// "enum_e { enum_e::a, enum_e::b }" => "enum_e { a, b }".
function simplify_enum_declarations(string $string) : string
{
    $simplified = preg_replace('/(<a class="el".*?>)[^<]*?_e::(.+?<\/a>)/', "$1$2", $string);

    return $simplified;
}
