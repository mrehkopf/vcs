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

def parse_dom(html:str)->BeautifulSoup:
    return BeautifulSoup(html, "html.parser")

def remove_non_breaking_spaces(html:str)->str:
    html = html.replace("&nbsp;", "")
    html = html.replace("&#160;", "")

    return html

def simplify_enum_declarations(html:str)->str:
    dom = parse_dom(html)

    # E.g. "enum_e { enum_e::a, enum_e::b }" => "enum_e { a, b }".
    # Note: Only enum declarations in <a></a> contain the "xxx::" part we want to remove.
    enumDeclNodes = dom.select("table.memberdecls tr[class^=memitem] > td.memItemRight > a")
    enumDeclNodes = filter_leaf_nodes(enumDeclNodes)
    for node in enumDeclNodes:
        node.string = re.sub(r'.*?::', '', node.string)

    enumDeclNodes = dom.select("table.memberdecls tr[class^=memitem] > td.memItemRight")

    return str(dom)

# Returns the function definition table in the given BeautifulSoup DOM object;
# or None if the DOM doesn't include that table. 
def get_function_table(dom:BeautifulSoup):
    # FIXME: What if the page has both the func-members and pub-methods tables?
    fnTableAnchor = dom.select("table.memberdecls a[name=func-members],"
                               "table.memberdecls a[name=pub-methods]")

    if (not fnTableAnchor): return None

    fnTableAnchor = fnTableAnchor[0]
    parent = fnTableAnchor
    for x in range(4): parent = parent.parent
    return parent

def strip_unwanted_whitespace(html:str)->str:
    # For function return declarations.
    # E.g. "std::vector< std::pair< unsigned, double >>"
    #   => "std::vector<std::pair<unsigned, double>>".
    html = re.sub(r'([^\s]?)&lt; ', '\g<1>&lt;', html)
    html = re.sub(r'(.*?) &gt;', '\g<1>&gt;', html)

    # For function return declarations; e.g. "void *" => "void*".
    html = re.sub(r' +(\*|&amp;) *<\/td>', '\g<1></td>', html)

    # For function return declarations; e.g. "std::vector<int *>" => "std::vector<int*>".
    html = re.sub(r'(&lt;.*?) ((\*|&amp;)&gt;)', '\g<1>\g<2>', html)

    dom = parse_dom(html)

    # Strip the space between a function's name and its list of parameters.
    # E.g. "function (int x)" => "function(int x)".
    functionTable = get_function_table(dom)
    if (functionTable):
        nodes = (functionTable.select("table.memberdecls tr[class^=memitem] > td.memItemRight"))
        for node in nodes:
            node.contents[1].replaceWith(node.contents[1].lstrip())

    return str(dom)

def remove_unwanted_elements(html:str)->str:
    dom = parse_dom(html)

    # Remove certain labels like "strong" next to member names.
    prototypeLabels = dom.select(".memproto > table.mlabels .mlabels-right .mlabel")
    for node in prototypeLabels:
        if node.string in ["strong", "virtual"]:
            node.decompose()

    # Simplify "(void)" in function declarations to "()"."
    functionTable = get_function_table(dom)
    if (functionTable):
        nodes = (functionTable.select("table.memberdecls tr[class^=memitem] > td.memItemRight"))
        for node in nodes:
            node.contents[1].replaceWith(node.contents[1].replace("(void)", "()"))

    # Simplify "(void)" in function documentation to "()"."
    paramNodes = filter_leaf_nodes(dom.select(".paramtype"))
    for node in paramNodes:
        if (node.string == "void"):
            node.string = ""

    return str(dom)

def singly_capitalize(html:str)->str:
    dom = parse_dom(html)

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

    dom = parse_dom(html)

    titleNode = dom.select("#doc-content > .header .title")
    if (not titleNode): return str(dom)

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
    dom = parse_dom(html)

    fragmentNodes = dom.select("div.fragment")
    for node in fragmentNodes:
        node.name = "pre"

    codeNodes = dom.select("pre.fragment > div.line")
    for node in codeNodes:
        node.name = "code"

    return str(dom)

# Formats function signatures a little better.
def betterize_memnames(html:str)->str:
    dom = parse_dom(html)

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
            nameSpan = dom.new_tag("span")
            nameSpan["class"] = "vcs-member-name"
            nameSpan.string = memName.text.strip()
            nameList.append(nameSpan)

        nameList.append(NavigableString("("))

        for typeNode, nameNode in zip(paramTypeNodes, paramNameNodes):
            typeLink = typeNode.select("a")

            if (typeLink):
                typeLink[0]["class"].append("vcs-param-type")
                nameList.append(typeLink[0])
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

for filename in os.listdir("./html/"):
    filename = "./html/" + filename

    if filename.endswith('.html'):
        htmlFile = open(filename, "r+")
        html = htmlFile.read()

        # Wrap in <pre> to conserve whitespace in code fragments.
        html = "<pre>" + html + "</pre>"

        html = remove_non_breaking_spaces(html)
        html = strip_unwanted_whitespace(html)
        html = standardize_code_elements(html)
        html = simplify_enum_declarations(html)
        html = remove_unwanted_elements(html)
        html = recreate_reference_page_title(html)
        html = singly_capitalize(html)
        html = betterize_memnames(html)

        # Remove the document-wrapping <pre> tag.
        html = html[5:-6]

        htmlFile.seek(0)
        htmlFile.write(html)
        htmlFile.truncate()
        htmlFile.close()

print("Reduced the output's HTML")
