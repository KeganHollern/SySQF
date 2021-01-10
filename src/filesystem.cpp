#include "filesystem.h"
#include "fileio.h"
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fstream>

void sqf::filesystem::addPathMappingInternal(std::filesystem::path virt, std::filesystem::path phy) {
    std::vector<std::string> virtElements;

    for (auto& el : virt) { //Split path into elements
		if (el.string().empty())
		{
			continue;
		}
        virtElements.emplace_back(el.string());
    }

    auto found = m_virtualphysicalmap.find(virtElements[0]);
    auto curIter = m_virtualphysicalmap.end();
    bool first = true;
    for (auto& it : virtElements) {
        if (first) { //first element
            first = false; //this is ugly. But comparing iterators doesn't work
            curIter = m_virtualphysicalmap.find(it);
            if (curIter == m_virtualphysicalmap.end())
                curIter = m_virtualphysicalmap.insert({ it, pathElement{} }).first;
            continue;
        }
        auto& curEl = curIter->second;
        curIter = curEl.subPaths.find(it);
        if (curIter == curEl.subPaths.end())
            curIter = curEl.subPaths.insert({ it, pathElement{} }).first;
    }

    curIter->second.physicalPath = phy;
}

std::optional<std::filesystem::path> sqf::filesystem::resolvePath(std::filesystem::path virt) {
    std::vector<std::string> virtElements;

    for (auto& el : virt) { //Split path into elements
        virtElements.emplace_back(el.string());
    }

    if (virtElements.front() == "\\") //We already know it's a global path. We don't want starting backslash
        virtElements.erase(virtElements.begin());

    std::vector<std::map<std::string, pathElement>::iterator> pathStack; //In case we need to walk back upwards

    auto curIter = m_virtualphysicalmap.end();
    bool first = true;
    for (auto& it : virtElements) {
        if (first) { //first element needs special handling as it comes directly from the map
            first = false; //this is ugly. But comparing iterators doesn't work
            curIter = m_virtualphysicalmap.find(it);
            if (curIter == m_virtualphysicalmap.end())
                return {}; //if we didn't find the starting element, we won't find any of the next elements either
            pathStack.emplace_back(curIter);
            continue;
        }
        auto& curEl = curIter->second;
        curIter = curEl.subPaths.find(it);
        if (curIter == curEl.subPaths.end())
            break; //not found
        pathStack.emplace_back(curIter);
    }

    virtElements.erase(virtElements.begin(), virtElements.begin() + pathStack.size()); //Remove elements we resolved to get a list of leftovers


    //walk up stack until we find a phys path.

    while (!pathStack.back()->second.physicalPath) {
        virtElements.emplace_back(pathStack.back()->first);
        pathStack.pop_back();
        if (pathStack.empty())
            return {}; //whole stack didn't have physical path
    }

    //build full path to file
    auto curPath = *pathStack.back()->second.physicalPath;
    for (auto& it : virtElements)
        curPath = curPath / it;

    return curPath;
}

std::optional<std::string> sqf::filesystem::try_get_physical_path(std::string_view virt, std::string_view current)
{
	if (virt.empty())
	{
		return {};
	}
#if !defined(FILESYSTEM_DISABLE_DISALLOW)
	if (mdisallow)
	{
		return {};
	}
#endif
	std::string virtMapping;
    if (virt.front() != '\\') { //It's a local path
        auto parentDirectory = std::filesystem::path(current).parent_path(); //Get parent of current file
        auto wantedFile = (parentDirectory / virt).lexically_normal();

        if (virt.find("..") != std::string::npos) {//need to check against physical boundary
            //#TODO implement this as a tree lookup
            auto found = std::find_if(m_physicalboundaries.begin(), m_physicalboundaries.end(), [search = wantedFile.string()](std::string_view it) -> bool {
                return search.find(it) != std::string::npos;
            });

            if (found == m_physicalboundaries.end())
                return {}; //boundary violation
        }

		if (std::filesystem::exists(wantedFile))
		{
			auto absolute = std::filesystem::absolute(wantedFile);
			return absolute.string();
		}
        return {}; //file doesn't exist
    } else { //global path
        auto resolved = resolvePath(virt);
        if (resolved) {
            if (std::filesystem::exists(*resolved))
			{
				auto absolute = std::filesystem::absolute(*resolved);
				return absolute.string();
			}
            return {}; //file doesn't exist
        }
        return {}; //can't resolve file
    }
}

void sqf::filesystem::add_allowed_physical(std::string_view phys)
{
	auto san_phys = sanitize(phys);
	m_physicalboundaries.push_back(san_phys);
}

void sqf::filesystem::add_mapping(std::string_view virt, std::string_view phys)
{
	auto san_virt = sanitize(virt);
	auto san_phys = sanitize(phys);
	m_physicalboundaries.push_back(san_phys);
	m_virtualpaths.push_back(san_virt);
    addPathMappingInternal(san_virt, san_phys);
}

void sqf::filesystem::add_mapping_auto(std::string_view phys)
{
	const std::filesystem::path ignoreGit(".git");
	const std::filesystem::path ignoreSvn(".svn");

	//recursively search for pboprefix
	for (auto i = std::filesystem::recursive_directory_iterator(phys, std::filesystem::directory_options::follow_directory_symlink);
		i != std::filesystem::recursive_directory_iterator();
		++i)
	{
		if (i->is_directory() && (i->path().filename() == ignoreGit || i->path().filename() == ignoreSvn))
		{
			i.disable_recursion_pending(); //Don't recurse into that directory
			continue;
		}
		if (!i->is_regular_file()) continue;

		if (i->path().filename() == "$PBOPREFIX$")
		{

			std::ifstream prefixFile(i->path());
			std::string prefix;
			std::getline(prefixFile, prefix);
			prefixFile.close();
			addPathMappingInternal(prefix, i->path().parent_path());
			add_mapping(prefix, i->path().parent_path().string());
		}
	}
}