Score* FileReader_read(const char* const filename) {
    xmlDocPtr doc = xmlReadFile(filename, NULL, 0);
    if (!doc) {
        die("Failed to parse input file '%s'", filename);
    }

    xmlNode* rootNode = xmlDocGetRootElement(doc);
    printf("rootNode->name = '%s'\n", rootNode->name);
    xmlFreeDoc(doc);

    return NULL;
}
