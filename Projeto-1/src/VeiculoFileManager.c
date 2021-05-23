#include "VeiculoFileManager.h"


// (Static) Parses first CSV file line to the header struct
// Return value: A pointer to the built struct (LinhaHeader *)
static VeiculoHeader *readVeiculoHeader(FILE *fpVeiculo) {
    if (fpVeiculo) {
        char *lineRead = readline(fpVeiculo);
        if (lineRead) {
            VeiculoHeader *newHeader = (VeiculoHeader *) malloc(sizeof *newHeader);

            newHeader->fileStatus = CONSISTENT_FILE;
            newHeader->regNumber = 0;
            newHeader->byteNextReg = 0;
            newHeader->removedRegNum = 0;

            // Tracks the original pointing address so it can be free'd later on
            char *trackReference = lineRead;

            newHeader->prefixDescription = strdup(strsep(&lineRead, DELIM));
            newHeader->dateDescription = strdup(strsep(&lineRead, DELIM));
            newHeader->seatsDescription = strdup(strsep(&lineRead, DELIM));
            newHeader->lineDescription = strdup(strsep(&lineRead, DELIM));
            newHeader->modelDescription = strdup(strsep(&lineRead, DELIM));
            newHeader->categoryDescription = strdup(strsep(&lineRead, LINE_BREAK));
            free(trackReference);
            return newHeader;
        }
        return NULL;
    }
    return NULL;
}

// (Static) Parses current CSV file line to the registry struct passed as parameter
// Return value: If everything succeeded (boolean)

static bool readVeiculoRegistryCsv(FILE *fpVeiculo, VeiculoData *newData) {
    if (fpVeiculo) {
        char *lineRead = readline(fpVeiculo);
        readVeiculoRegistry(lineRead, newData);
    }
    return false;
}


bool readVeiculoRegistry(char *stringVeiculo, VeiculoData *newData) {
    if (stringVeiculo && strlen(stringVeiculo) > 0 && stringVeiculo[0] != -1) {
        char *trackReference = stringVeiculo;
        char *auxString = NULL;

        if (stringVeiculo[0] == '*') {
            newData->isRemoved = REMOVED_REGISTRY;
            stringVeiculo++;
        }
        else
            newData->isRemoved = VALID_REGISTRY;

        auxString = strsep(&stringVeiculo, DELIM);
        newData->prefix = strdup(auxString);

        auxString = strsep(&stringVeiculo, DELIM);
        if (strcmp(auxString, "NULO")) {
            newData->date = strdup(auxString);
        } else {
            newData->date = (char*)calloc(sizeof(char), DATE_SIZE);
            strncpy(newData->date, "@@@@@@@@@@", DATE_SIZE);
            newData->date[0] = '\0';
        }
        newData->seatsNumber = atoi(strsep(&(stringVeiculo), DELIM));
        newData->linhaCode = atoi(strsep(&(stringVeiculo), DELIM));
        
        auxString = strsep(&stringVeiculo, DELIM);
        if (strcmp(auxString, "NULO")) {
            newData->model = strdup(auxString);
            newData->modelSize = strlen(newData->model);
        } else {
            newData->modelSize = 0;
        }
        auxString = strsep(&stringVeiculo, LINE_BREAK);
        if (strcmp(auxString, "NULO")) {
            newData->category = strdup(auxString);
            newData->categorySize = strlen(newData->category);
        } else {
            newData->categorySize = 0;
        }

        free(trackReference);

        newData->regSize = newData->modelSize + newData->categorySize + VEICULO_FIXED_SIZE;
        return true;
    }
    free(stringVeiculo);
    return false;
}

// (Static) Writes each header field in sequence to the binary file
// Return value: If all data were written as expected (boolean)
bool writeVeiculoHeaderOnBinary(FILE *binFile, VeiculoHeader *headerStruct) {
    if (binFile && headerStruct) {
        size_t bytesWritten = 0;

        // Fixed size fields (82 bytes)
        bytesWritten += fwrite(&headerStruct->fileStatus, sizeof(char), 1, binFile);
        bytesWritten += fwrite(&headerStruct->byteNextReg, sizeof(int64_t), 1, binFile);
        bytesWritten += fwrite(&headerStruct->regNumber, sizeof(int32_t), 1, binFile);
        bytesWritten += fwrite(&headerStruct->removedRegNum, sizeof(int32_t), 1, binFile);
        bytesWritten += fwrite(headerStruct->prefixDescription, sizeof(char), PREFIX_DESC_SIZE, binFile);
        bytesWritten += fwrite(headerStruct->dateDescription, sizeof(char), DATE_DESC_SIZE, binFile);
        bytesWritten += fwrite(headerStruct->seatsDescription, sizeof(char), SEATS_DESC_SIZE, binFile);
        bytesWritten += fwrite(headerStruct->lineDescription, sizeof(char), LINE_DESC_SIZE, binFile);
        bytesWritten += fwrite(headerStruct->modelDescription, sizeof(char), MODEL_DESC_SIZE, binFile);
        bytesWritten += fwrite(headerStruct->categoryDescription, sizeof(char), CATEGORY_DESC_SIZE, binFile);

        if (bytesWritten != VEICULO_HEADER_SIZE)
            return false;

        return true;
    }
    return false;
}

// (Static) Writes each registry field in sequence to the binary file
// Return value: If all data were written (boolean)
bool writeVeiculoRegistryOnBinary(FILE *binFile, VeiculoData *registryStruct) {
    if (binFile && registryStruct) {
        size_t bytesWritten = 0;

        // Fixed size fields (13 bytes)
        bytesWritten += fwrite(&registryStruct->isRemoved, sizeof(char), 1, binFile);
        bytesWritten += fwrite(&registryStruct->regSize, sizeof(int32_t), 1, binFile);
        bytesWritten += fwrite(registryStruct->prefix, sizeof(char), PREFIX_SIZE, binFile);
        bytesWritten += fwrite(registryStruct->date, sizeof(char), DATE_SIZE, binFile);
        bytesWritten += fwrite(&registryStruct->seatsNumber, sizeof(int32_t), 1, binFile);
        bytesWritten += fwrite(&registryStruct->linhaCode, sizeof(int32_t), 1, binFile);
        

        // Dynamic size fields
        bytesWritten += fwrite(&registryStruct->modelSize, sizeof(int32_t), 1, binFile);
        if (registryStruct->modelSize > 0)
            bytesWritten += fwrite(registryStruct->model, sizeof(char), registryStruct->modelSize, binFile);
        bytesWritten += fwrite(&registryStruct->categorySize, sizeof(int32_t), 1, binFile);
        if (registryStruct->categorySize > 0)
            bytesWritten += fwrite(registryStruct->category, sizeof(char), registryStruct->categorySize, binFile);

        // Checks if everything were written correctly (Register liquid size + isRemoved size + regSize size)
        if (bytesWritten != registryStruct->regSize + sizeof(char) + sizeof(int32_t))
            return false;

        return true;
    }
    return false;
}

// (Global) Frees everything related to the header struct (its pointer included)
// Return value: If everything succeeded as expected (boolean)
bool freeVeiculoHeader(VeiculoHeader **header) {
    if (header && *header) {
        if ((*header)->prefixDescription)
            free((*header)->prefixDescription);
        if ((*header)->dateDescription)
            free((*header)->dateDescription);
        if ((*header)->seatsDescription)
            free((*header)->seatsDescription);
        if ((*header)->lineDescription)
            free((*header)->lineDescription);
        if ((*header)->modelDescription)
            free((*header)->modelDescription);
        if ((*header)->categoryDescription)
            free((*header)->categoryDescription);
            
        free(*header);
        return true;
    }
    return false;
}

// (Static) Frees everything inside registry data struct (its pointer not included)
// Return value: If everything succeeded as expected (boolean)
bool freeVeiculoData(VeiculoData *data) {
    if (data) {
        free(data->prefix);
        free(data->date);
        if (data->modelSize > 0)
            free(data->model);
        if (data->categorySize > 0)
            free(data->category);
        return true;
    }
    return false;
}

// (Global) Reads a CSV file of category "Linhas" and write its respective binary file
// Return value: File pointer to binary file (FILE *)
bool writeVeiculoBinary(char *csvFilename, char *binFilename) {
    if (csvFilename && strlen(csvFilename) > 0) {
        FILE *csvFile = fopen(csvFilename, "r");
        if (!csvFile) {
            return false;
        }

        VeiculoHeader *fileHeader = readVeiculoHeader(csvFile);
        if (!fileHeader) {
            return false;
        }

        FILE *binFile = fopen(binFilename, "wb+");
        if (!binFile) {
            return false;
        }
        
        // Writes 82 bytes as a placeholder for the header
        // Signing file as "inconsistent" (0)
        fwrite(INCONSISTENT_FILE, sizeof(char), 1, binFile);
        fwrite("@", sizeof(char), VEICULO_HEADER_SIZE-1, binFile);
        
        // Read each registry from CSV file and write it to binary
        VeiculoData *newRegistry = (VeiculoData *) malloc(sizeof *newRegistry);
        char EOFFlag;
        do {
            EOFFlag = fgetc(csvFile);
            if (EOFFlag != EOF) {
                fseek(csvFile, -1, SEEK_CUR);
                readVeiculoRegistryCsv(csvFile, newRegistry);
                writeVeiculoRegistryOnBinary(binFile, newRegistry);
                if (newRegistry->isRemoved == REMOVED_REGISTRY)
                    fileHeader->removedRegNum++; 
                else
                    fileHeader->regNumber++;
                freeVeiculoData(newRegistry);
            }
        } while (EOFFlag != EOF);
        fclose(csvFile);
        free(newRegistry);

        // Rewind into binary file and write the header struct
        // Signing file as "consistent" (1)
        fileHeader->byteNextReg = (int64_t) ftell(binFile);
        rewind(binFile);
        writeVeiculoHeaderOnBinary(binFile, fileHeader);
        freeVeiculoHeader(&fileHeader);
        fclose(binFile);
        return true;
    }
    return false;
}

// (Global) Reads and builds the header struct from the binary file
// Return value: A pointer for the built struct (LinhaHeader *)
VeiculoHeader *loadVeiculoBinaryHeader(FILE *binFile) {
    if (binFile) {
        size_t bytesRead = 0;

        // Check file consistency
        char fileStatus;
        bytesRead += fread(&fileStatus, 1, sizeof(char), binFile);
        if (fileStatus == INCONSISTENT_FILE[0] || fileStatus == EOF || bytesRead <= 0)
            return NULL;

        VeiculoHeader *newHeader = (VeiculoHeader *) malloc(sizeof *newHeader);
        newHeader->fileStatus = fileStatus;

        // Load numeric file statistics
        bytesRead += fread(&newHeader->byteNextReg, 1, sizeof(int64_t), binFile);
        bytesRead += fread(&newHeader->regNumber, 1, sizeof(int32_t), binFile);
        bytesRead += fread(&newHeader->removedRegNum, 1, sizeof(int32_t), binFile);

        // Load header strings
        newHeader->prefixDescription = (char *) malloc(PREFIX_DESC_SIZE*sizeof(char));
        bytesRead += fread(newHeader->prefixDescription, sizeof(char), PREFIX_DESC_SIZE, binFile);
        newHeader->dateDescription = (char *) malloc(DATE_DESC_SIZE*sizeof(char));
        bytesRead += fread(newHeader->dateDescription, sizeof(char), DATE_DESC_SIZE, binFile);
        newHeader->seatsDescription = (char *) malloc(SEATS_DESC_SIZE*sizeof(char));
        bytesRead += fread(newHeader->seatsDescription, sizeof(char), SEATS_DESC_SIZE, binFile);
        newHeader->lineDescription = (char *) malloc(LINE_DESC_SIZE*sizeof(char));
        bytesRead += fread(newHeader->lineDescription, sizeof(char), LINE_DESC_SIZE, binFile);
        newHeader->modelDescription = (char *) malloc(MODEL_DESC_SIZE*sizeof(char));
        bytesRead += fread(newHeader->modelDescription, sizeof(char), MODEL_DESC_SIZE, binFile);
        newHeader->categoryDescription = (char *) malloc(CATEGORY_DESC_SIZE*sizeof(char));
        bytesRead += fread(newHeader->categoryDescription, sizeof(char), CATEGORY_DESC_SIZE, binFile);
        
        if (bytesRead != VEICULO_HEADER_SIZE) {
            freeVeiculoHeader(&newHeader);
            return NULL;
        }
        return newHeader;

    }
    return NULL;
}

// (Static) Reads and builds the registry struct (to an already existent struct) from the binary file
// Return value: If the read succeeded as expected (boolean)
bool loadVeiculoBinaryRegistry(FILE *binFile, VeiculoData *registryStruct) {
    if (binFile && registryStruct) {
        size_t bytesRead = 0;
        
        // Check registry removed status
        int32_t regSize;
        char registryStatus;
        if(!fread(&registryStatus, 1, sizeof(char), binFile)) {
            return false;
        }
        fread(&regSize, 1, sizeof(int32_t), binFile);

        registryStruct->isRemoved = registryStatus;
        registryStruct->regSize = regSize;
        if (registryStatus == REMOVED_REGISTRY) {
            fseek(binFile, regSize - 5, SEEK_CUR); //need to fseek regsize-5 ahead of current posistion because some bytes were already read
            return true;
        }

        // Load fixed size fields
        registryStruct->prefix = (char *) malloc(PREFIX_SIZE*sizeof(char));
        bytesRead += fread(registryStruct->prefix, sizeof(char), PREFIX_SIZE, binFile);
        
        registryStruct->date = (char *) malloc(DATE_SIZE*sizeof(char));
        bytesRead += fread(registryStruct->date, sizeof(char), DATE_SIZE, binFile);

        bytesRead += fread(&registryStruct->seatsNumber, 1, sizeof(int32_t), binFile);

        bytesRead += fread(&registryStruct->linhaCode, 1, sizeof(int32_t), binFile);

        // Load dynamic size fields
        bytesRead += fread(&registryStruct->modelSize, 1, sizeof(int32_t), binFile);
        if (registryStruct->modelSize > 0) {
            registryStruct->model = (char *) malloc(registryStruct->modelSize*sizeof(char));
            bytesRead += fread(registryStruct->model, sizeof(char), registryStruct->modelSize, binFile);
        }
        bytesRead += fread(&registryStruct->categorySize, 1, sizeof(int32_t), binFile);
        if (registryStruct->categorySize > 0) {
            registryStruct->category = (char *) malloc(registryStruct->categorySize*sizeof(char));
            bytesRead += fread(registryStruct->category, sizeof(char), registryStruct->categorySize, binFile);
        }

        // Check read status
        if (bytesRead != VEICULO_FIXED_SIZE + registryStruct->modelSize + registryStruct->categorySize) {
            freeVeiculoData(registryStruct);
            return false;
        }
        return true;
    }
    
    return false;
}


void printVeiculoHeader(VeiculoHeader *header) {
    if (header) {
        printf("fileStatus: %c\n", header->fileStatus);
        printf("byteNextReg: %" PRId64 "\n", header->byteNextReg);
        printf("regNumber: %d\n", header->regNumber);
        printf("removedRegNum: %d\n", header->removedRegNum);
        printf("prefixDescription: %.*s\n", PREFIX_DESC_SIZE, header->prefixDescription);
        printf("dateDescription: %.*s\n", DATE_DESC_SIZE, header->dateDescription);
        printf("seatsDescription: %.*s\n", SEATS_DESC_SIZE, header->seatsDescription);
        printf("lineDescription: %.*s\n", LINE_DESC_SIZE, header->lineDescription);
        printf("modelDescription: %.*s\n", MODEL_DESC_SIZE, header->modelDescription);
        printf("categoryDescription: %.*s\n", CATEGORY_DESC_SIZE, header->categoryDescription);
    }
}

/*
void printVeiculoRegistry(VeiculoData *data) {
    printf("isRemoved: %c\n", data->isRemoved);
    printf("regSize: %d\n", data->regSize);
    printf("prefix: %.*s\n", PREFIX_SIZE, data->prefix);
    printf("date: %.*s\n", DATE_SIZE, data->date);
    printf("seatsNumber: %d\n", data->seatsNumber);
    printf("linhaCode: %d\n", data->linhaCode);
    printf("modelSize: %d\n", data->modelSize);
    printf("model: %.*s\n", data->modelSize, data->model);
    printf("categorySize: %d\n", data->categorySize);
    printf("category: %.*s\n", data->categorySize, data->category);
    printf("\n");
}
*/

void printVeiculoRegistry(VeiculoHeader *header, VeiculoData *registry) {
    if (header && registry) {
        printf("%.*s: %.*s\n", PREFIX_DESC_SIZE, header->prefixDescription, PREFIX_SIZE, registry->prefix);

        printf("%.*s: ", MODEL_DESC_SIZE, header->modelDescription);
        if (registry->modelSize == 0)
            printf("%s\n", NULL_FIELD);
        else
            printf("%.*s\n", registry->modelSize, registry->model);

        printf("%.*s: ", CATEGORY_DESC_SIZE, header->categoryDescription);
        if (registry->categorySize == 0)
            printf("%s\n", NULL_FIELD);
        else
            printf("%.*s\n", registry->categorySize, registry->category);

        printf("%.*s: ", DATE_DESC_SIZE, header->dateDescription);
        if(registry->date[0] == '\0')
            printf("%s\n", NULL_FIELD);
        else {
            int year, month, day;
            year = atoi(strsep(&registry->date, "-"));
            month = atoi(strsep(&registry->date, "-"));
            day = atoi(strsep(&registry->date, LINE_BREAK));
            
            printf("%d de ", day);

            switch(month) {
                case 1:
                    printf("Janeiro");
                    break;
                case 2:
                    printf("Fevereiro");
                    break;
                case 3:
                    printf("Março");
                    break;
                case 4:
                    printf("Abril");
                    break;
                case 5:
                    printf("Maio");
                    break;
                case 6:
                    printf("Junho");
                    break;
                case 7:
                    printf("Julho");
                    break;
                case 8:
                    printf("Agosto");
                    break;
                case 9:
                    printf("Setembro");
                    break;
                case 10:
                    printf("Outubro");
                    break;
                case 11:
                    printf("Novembro");
                    break;
                case 12:
                    printf("Dezembro");
                    break;
                
            }

            printf(" de %d\n", year);
        }

        printf("%.*s: ", SEATS_DESC_SIZE, header->seatsDescription);
        if(registry->seatsNumber == -1)
            printf("%s\n", NULL_FIELD);
        else
            printf("%d\n", registry->seatsNumber);

        printf("\n");
    }
}
