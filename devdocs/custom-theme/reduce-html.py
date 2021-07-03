# 2021 Tarpeeksi Hyvae Soft
#
# Software: VCS
#
# Modifies (reduces) Doxygen-generated HTML files to make them compliant with
# VCS's custom Doxygen theme.

import re
import os
from bs4 import BeautifulSoup, NavigableString

def filter_leaf_nodes(nodes:list)->list:
    return [x for x in nodes if x.string != None]

def parse_html(html:str)->BeautifulSoup:
    return BeautifulSoup(html, "html.parser")

def remove_non_breaking_spaces(html:str)->str:
    html = html.replace("&nbsp;", "")
    html = html.replace("&#160;", "")
    return html

def simplify_enum_declarations(html:str)->str:
    dom = parse_html(html)

    # E.g. "enum_e { enum_e::a, enum_e::b }" => "enum_e { a, b }".
    # Note: Only enum declarations in <a></a> contain the "xxx::" part we want to remove.
    enumDeclNodes = dom.select("table.memberdecls tr[class^=memitem] > td.memItemRight > a")
    enumDeclNodes = filter_leaf_nodes(enumDeclNodes)
    for node in enumDeclNodes:
        node.string = re.sub(r'.*?::', '', node.string)

    enumDeclNodes = dom.select("table.memberdecls tr[class^=memitem] > td.memItemRight")

    return str(dom)

# Returns as a list all function declaration tables in the given Beautiful Soup
# object. If there are no such tables, returns an empty array.
def get_function_decl_tables(dom:BeautifulSoup):
    return filter(lambda x: x, [
        get_decl_table("Functions", dom),
        get_decl_table("Public member functions", dom),
        get_decl_table("Private member functions", dom)
    ])
    
# Returns a member declaration table of the given heading in the given Beautiful
# Soup object.
def get_decl_table(tableHeading:str, dom:BeautifulSoup):
    declTables = dom.select("#doc-content table.memberdecls")

    for table in declTables:
        heading = table.select(".groupheader")
        if heading:
            # Some headings just have a text node (<h2>Functions</h2>), while others
            # also have an anchor (<h2><a href="..."></a>Functions</h2>).
            for child in heading[0].contents:
                if child.string and child.string.lower().strip() == tableHeading.lower():
                    return table

    return None

def strip_unwanted_whitespace(html:str)->str:
    dom = parse_html(html)

    # Function declarations' return values and parameters.
    allElements = dom.select("div.memproto, tr[class^=memitem], div.header")
    for element in allElements:
        innerHTML = str(element)

        # E.g. "std::vector< std::pair< unsigned, double >>"
        #   => "std::vector<std::pair<unsigned, double>>".
        innerHTML = re.sub(r'([^\s]?)&lt; ', '\g<1>&lt;', innerHTML)
        innerHTML = re.sub(r'(.*?) &gt;', '\g<1>&gt;', innerHTML)

        # E.g. "void *" => "void*".
        innerHTML = re.sub(r' +(\*|&amp;) *<\/td>', '\g<1></td>', innerHTML)

        # E.g. "std::vector<int *>" => "std::vector<int*>".
        innerHTML = re.sub(r'(&lt;.*?) ((\*|&amp;)&gt;)', '\g<1>\g<2>', innerHTML)
        element.replaceWith(BeautifulSoup(str(innerHTML), "html.parser"))

    # Strip the space between a function's name and its list of parameters
    # in the function declaration table. E.g. "function (int x)" => "function(int x)".
    functionTables = get_function_decl_tables(dom)
    for table in functionTables:
        nodes = table.select("table.memberdecls tr[class^=memitem] > td.memItemRight")
        for node in nodes:
            node.contents[1].replaceWith(node.contents[1].lstrip())

    return str(dom)

def remove_unwanted_elements(html:str)->str:
    dom = parse_html(html)

    # Remove certain labels like "strong" next to member names.
    prototypeLabels = dom.select(".memproto > table.mlabels .mlabels-right .mlabel")
    for node in prototypeLabels:
        if node.string in ["strong", "virtual"]:
            node.decompose()

    # Simplify "(void)" in function declarations to "()".
    functionTables = get_function_decl_tables(dom)
    for table in functionTables:
        nodes = table.select("table.memberdecls tr[class^=memitem] > td.memItemRight")
        for node in nodes:
            node.contents[1].replaceWith(node.contents[1].replace("(void)", "()"))

    # Simplify "(void)" in function documentation to "()".
    paramNodes = filter_leaf_nodes(dom.select(".paramtype"))
    for node in paramNodes:
        if (node.string == "void"):
            node.string = ""

    return str(dom)

def singly_capitalize(html:str)->str:
    dom = parse_html(html)

    headingNodes = dom.select("h2.groupheader")
    for node in headingNodes:
        node.string = node.text.strip().capitalize()

    return str(dom)

# Inserts replacement formatting for the title of reference documents.
# E.g. "file.h File Reference" => "File reference > file.h" 
def recreate_reference_page_title(html:str)->str:
    targetPages = [
        "File Reference",
        "Struct Reference",
        "Class Reference",
        "Struct Template Reference",
        "Class Template Reference",
    ]

    dom = parse_html(html)

    titleNode = dom.select("#doc-content > .header .title")
    if (not titleNode):
        return str(dom)

    titleNode = titleNode[0]

    # E.g. "file.h File Reference" => ["file.h", "File Reference"]
    referentName = re.search('(.*?) ', titleNode.text).group(1)
    referrerName = re.search('.*? (.*)', titleNode.text).group(1)

    if (referrerName in targetPages):
        referentElement = dom.new_tag("span")
        separatorElement = dom.new_tag("span")
        referrerElement = dom.new_tag("span")

        referentElement["class"] = "vcs-referent"
        separatorElement["class"] = "vcs-separator"
        referrerElement["class"] = "vcs-referrer"

        referentElement.string = referentName
        referrerElement.string = referrerName.capitalize()
        titleNode.string = ""

        titleNode.append(referentElement)
        titleNode.append(separatorElement)
        titleNode.append(referrerElement)
    else:
        titleNode.string = (referentName + " " + referrerName).capitalize()

    return str(dom)

def standardize_code_elements(html:str)->str:
    dom = parse_html(html)

    fragmentNodes = dom.select("div.fragment")
    for node in fragmentNodes:
        node.name = "pre"

    codeNodes = dom.select("pre.fragment > div.line")
    for node in codeNodes:
        node.name = "code"

    return str(dom)

# Formats function signatures a little better.
def betterize_memnames(html:str)->str:
    dom = parse_html(html)

    memnameNodes = dom.select("table.memname")
    for node in memnameNodes:

        # Try to only target function memnames.
        if node.parent["class"][0] != "memproto":
            continue

        nameList = []

        paramTypeNodes = node.select(".paramtype")
        paramNameNodes = node.select(".paramname")
        assert len(paramTypeNodes) == len(paramNameNodes)
        
        memNameLink = node.select(".memname > a")
        memName = node.select(".memname")[0]

        if memNameLink:
            split = memName.text.strip().split(memNameLink[0].text)
            nameList.append(NavigableString(split[0]))
            nameList.append(memNameLink[0])
            nameList.append(NavigableString(split[1]))
        else:
            split = memName.text.strip().rpartition(" ")
            retSpan = dom.new_tag("span")
            retSpan["class"] = "vcs-member-return"
            retSpan.string = split[0]
            nameSpan = dom.new_tag("span")
            nameSpan["class"] = "vcs-member-name"
            nameSpan.string = split[2]
            nameList.append(retSpan)
            nameList.append(nameSpan)

        if len(paramTypeNodes):
            nameList.append(NavigableString("("))

        for typeNode, nameNode in zip(paramTypeNodes, paramNameNodes):
            typeLink = typeNode.select("a")

            if typeLink:
                typeSpan = dom.new_tag("span")
                typeSpan["class"] = "vcs-param-type"
                for child in typeNode.contents:
                    if isinstance(child, str):
                        textNode = dom.new_tag("span")
                        textNode.string = child.string
                        typeSpan.append(textNode)
                    # Note: We assume all non-text nodes would be <a>.
                    else:
                        linkNode = dom.new_tag("a")
                        linkNode.string = child.text
                        linkNode["class"] = child["class"]
                        linkNode["href"] = child["href"]
                        typeSpan.append(linkNode)
                nameList.append(typeSpan)
            else:
                typeSpan = dom.new_tag("span")
                typeSpan["class"] = "vcs-param-type"
                typeSpan.string = typeNode.text.strip()
                nameList.append(typeSpan)

            if len(nameNode.text.strip()):
                nameList.append(NavigableString(" "))

                nameSpan = dom.new_tag("span")
                nameSpan["class"] = "vcs-param-name"
                nameSpan.string = nameNode.text.strip()
                nameList.append(nameSpan)

                nameList.append(NavigableString(" "))

        # Remove the last, unnecessary separating space.
        if len(paramTypeNodes):
            nameList.pop()
            nameList.append(NavigableString(")"))

        new = dom.new_tag("span")
        new["class"] = "memname"
        for l in nameList:
            new.append(l)

        node.parent.append(new)
        node.decompose()

    return str(dom)

# Moves .memitem elements' "See also" section from .memdoc to the parent
# .memitem. This allows us to style the section as a .memitem footer in CSS. 
def footerize_memitem_see_section(html:str)->str:
    dom = parse_html(html)

    memItems = dom.select("#doc-content .memitem")

    for memItem in memItems:
        seeSection = memItem.select(".memdoc > .section.see")
        if seeSection:
            term = seeSection[0].select("dt")
            if term:
                # The text is expected to already be "See also", we add the colon.
                term[0].string = "See also:"
            memItem.append(seeSection[0])

    return str(dom)

# Create specific formatting for event documentation. Otherwise, events are lumped
# with variable declarations.
def specialize_event_documentation(html:str)->str:
    dom = parse_html(html)

    variableTable = get_decl_table("Variables", dom)

    if not variableTable:
        return str(dom)

    eventDecls = []
    allDecls = variableTable.select("tr[class^=memitem]")
    for decl in allDecls:
        left = decl.select("td.memItemLeft")
        if left and left[0].text.strip().startswith("vcs_event_c<"):
            eventDecls.append(decl)

    if not eventDecls:
        return str(dom)

    # Create a table for event declarations.
    eventsTable = dom.new_tag("table")
    eventsTable["class"] = "memberdecls"
    eventsTableBody = dom.new_tag("tbody")
    heading = dom.new_tag("tr")
    heading["class"] = "heading"
    td = dom.new_tag("td")
    td["colspan"] = "2"
    h2 = dom.new_tag("h2")
    h2["class"] = "groupheader"
    h2.string = "Events"
    td.append(h2)
    heading.append(td)
    eventsTableBody.append(heading)
    eventsTable.append(eventsTableBody)
    variableTable.insert_after(eventsTable)

    # Remove "vcs_event_c" from events' declarations.
    for event in eventDecls:
        eventsTableBody.append(event)
        leftItems = event.select(".memItemLeft")
        for left in leftItems:
            right = left.parent.select(".memItemRight")[0]
            for child in left.contents:
                if child.string:
                    # FIXME: This will probably fail if vcs_event_c is a link rather than a text node.
                    child.string.replace_with(child.string.replace("vcs_event_c<", "<"))

    # Move the events from the variables table to the events table.
    for event in eventDecls:
        eventsTableBody.append(event)

    # The variables table can be removed if it's now empty.
    if not variableTable.select("tr[class^=memitem]"):
        variableTable.decompose()

    # Find all event documentation elements.
    eventDocElements = []
    allMemItems = dom.select("#doc-content div.memitem")
    for item in allMemItems:
        memName = item.select("span.memname")
        if memName:
            for child in memName[0].contents:
                if child.string and child.string.startswith("vcs_event_c<"):
                    # FIXME: This will probably fail if vcs_event_c is a link rather than a text node.
                    child.string.replace_with(child.string.replace("vcs_event_c<", "<"))
                    eventDocElements.append(item)
                    break

    variableHeading = dom.select("#doc-content h2.groupheader")
    for heading in variableHeading:
        if heading.string and heading.string == "Variable documentation":
            variableHeading = heading
            break

    # If we couldn't find the specific heading.
    if isinstance(variableHeading, list):
        return str(dom)
    
    # Create a heading for event documentation.
    eventsHeading = dom.new_tag("h2")
    eventsHeading["class"] = "groupheader"
    eventsHeading.string = "Event documentation"
    variableHeading.insert_before(eventsHeading)

    # Move event documentation under its own heading.
    for item in eventDocElements:
        itemAnchor = item.find_previous().find_previous().find_previous()
        assert itemAnchor and itemAnchor.name == "h2"

        itemAnchor2 = itemAnchor.find_previous()
        assert itemAnchor2 and itemAnchor2.name == "a"

        eventsHeading.insert_after(item)
        eventsHeading.insert_after(itemAnchor)
        eventsHeading.insert_after(itemAnchor2)

    # Remove the "Variable documentation" heading if it's now empty.
    # Note: This code is untested and may not work as intended. Basically, a non-
    # empty heading should be followed by combinations of a/div/.memitem, which
    # constitute the heading's child documentation elements.
    if variableHeading.find_next().name != "a":
        variableHeading.decompose()

    return str(dom)

# Applies the given reducer functions to the HTML files in the given source
# directory (e.g. "./html/" - the path must end with "/").
def reduce_html(srcDir:str, reducerFunctions:list):
    for filename in os.listdir(srcDir):
        filename = srcDir + filename

        if filename.endswith(".html"):
            htmlFile = open(filename, "r+")
            html = htmlFile.read()

            # Wrap the document in <pre> to have the HTML parser conserve
            # whitespace in code fragments.
            html = "<pre>" + html + "</pre>"

            for reducerFn in reducerFunctions:
                html = reducerFn(html)

            # Remove the document-wrapping <pre> tag.
            html = html[5:-6]

            htmlFile.seek(0)
            htmlFile.write(html)
            htmlFile.truncate()
            htmlFile.close()

print("Reducing the output's HTML...")
reduce_html("./html/", [
    remove_non_breaking_spaces,
    strip_unwanted_whitespace,
    standardize_code_elements,
    simplify_enum_declarations,
    remove_unwanted_elements,
    recreate_reference_page_title,
    singly_capitalize,
    betterize_memnames,
    footerize_memitem_see_section,
    specialize_event_documentation,
])
