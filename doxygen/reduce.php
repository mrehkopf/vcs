<?php

/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 * Modifies (reduces) Doxygen-generated HTML files to make them compliant with
 * VCS's custom Doxygen theme.
 * 
 */

$htmlFiles = glob("./html/*.html");

// The reducer functions to apply to the input HTML. The functions will be called
// on the input in the order given here.
$reducerChain = [
    "remove_non_breaking_spaces",
    "remove_unnecessary_spaces",
    "simplify_enum_declarations",
    "mark_unnecessary_elements",
    "standardize_code_elements",
];

foreach ($htmlFiles as $filename)
{
    $reducedContents = file_get_contents($filename);

    foreach ($reducerChain as $reducerFn)
    {
        $reducedContents = $reducerFn($reducedContents);
    }

    file_put_contents($filename, $reducedContents);
}

echo "Reduced the output.\n";

function remove_non_breaking_spaces(string $html) : string
{
    $replacements = [
        "&nbsp;"=>" ",
        "&#160;"=>" ",
    ];

    return strtr($html, $replacements);
}

function remove_unnecessary_spaces(string $html) : string
{
    $html = str_replace('class="paramtype">void <',
                        'class="paramtype">void<',
                        $html);

    $html = preg_replace('/(<td class="memname">)(.*?) (<\/td>)/',
                        '$1$2$3',
                        $html);

    // E.g. "std::vector< std::pair< unsigned, double >>"
    //   => "std::vector<std::pair<unsigned, double>>".
    $html = preg_replace('/(?)&lt; /',
                        '$1&lt;',
                        $html);
    $html = preg_replace('/(?) &gt;/',
                        '$1&gt;',
                        $html);

    return $html;
}

// "enum_e { enum_e::a, enum_e::b }" => "enum_e { a, b }".
function simplify_enum_declarations(string $html) : string
{
    return preg_replace('/(<a class="el".*?>)([^<]*?_e::)(.+?<\/a>)/',
                        '$1<span class="vcs-enum-namespace">$2</span>$3',
                        $html);
}

function mark_unnecessary_elements(string $html) : string
{
    $html = str_replace('<span class="mlabel">strong</span>',
                        '<span class="mlabel vcs-unnecessary-element">strong</span>',
                        $html);

    $html = str_replace('<span class="mlabel">virtual</span>',
                        '<span class="mlabel vcs-unnecessary-element">virtual</span>',
                        $html);

    $html = str_replace('<td class="paramtype">void</td>',
                        '<td class="paramtype vcs-unnecessary-element">void</td>',
                        $html);

    // Declaration of functions taking void as argument (we can omit the "void").
    $html = preg_replace('/(<td class="memItemRight".*?><a class="el".*?>.*?<\/a> )\(void\)(.*?<\/td>)/',
                        '$1(<span class="vcs-unnecessary-element">void</span>)$2',
                        $html);

    // Declarations of virtual functions; we can omit the "virtual" from the return value.
    $html = preg_replace('/(<td class="memItemLeft".*?>)(virtual)/',
                        '$1<span class="vcs-unnecessary-element">$2</span>',
                        $html);

    $html = preg_replace('/(<td class="memname">)(virtual)/',
                        '$1<span class="vcs-unnecessary-element">$2</span>',
                        $html);

    return $html;
}

// Makes...
//
// <div class="fragment">
//   <div class="line"></div>
//   <div class="line"></div>
//   ...
// </div>
//
// ...into...
//
// <pre class="fragment">
//   <code class="line"></code>
//   <code class="line"></code>
//   ...
// </pre>
//
// Uses some kludge HTML regex parsing.
function standardize_code_elements(string $html) : string
{
    $html = str_replace('<div class="fragment">',
                        '<pre class="fragment">',
                        $html);

    // It seems that the closing tag for <div class="fragment"> is followed
    // immediately by <!-- fragment -->, for whatever reason. But we'll use
    // that to identify the closing tag.
    $html = str_replace('</div><!-- fragment -->',
                        '</pre><!-- fragment -->',
                        $html);

    // Note: This regex will fail if the <div class="line"> elements contain
    // other <div> elements. But I'm not aware that Doxygen inserts child
    // <div>s into these - seems to just use <span> and text nodes.
    $html = preg_replace('/<div class="line">(.*?)<\/div>/',
                         '<code class="line">$1</code>',
                         $html);
                        
    return $html;
}
