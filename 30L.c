#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_NAME_LENGTH 20
#define INITIAL_TABLE_SIZE 14999

typedef struct Lotto {
    int quantita;
    int scadenza;
    struct Lotto *prossimo;
} Lotto;

typedef struct NodoMagazzino {
    char *nome;
    int quantita_disponibile;
    Lotto *lotto;
    struct NodoMagazzino *prossimo;
} NodoMagazzino;

typedef struct Magazzino {
    NodoMagazzino **buckets; // Array di puntatori a NodoMagazzino
    int size; // Dimensione della tabella hash
    int count; // Numero di elementi nella tabella
} Magazzino;

typedef struct IngredienteRicetta {
	NodoMagazzino *riferimento_al_magazzino;
	int quantita_ricetta;
	struct IngredienteRicetta *prossimo;
} IngredienteRicetta;

typedef struct Ricetta {
    char *nome;
    int peso_totale;
    IngredienteRicetta *ingredienti;
    int ordini_in_sospeso;
    struct Ricetta *prossimo;
} Ricetta;

typedef struct LibroRicette {
    Ricetta **buckets; // Array di puntatori a Ricetta
    int size; // Dimensione della tabella hash
    int count; // Numero di ricette nella tabella
} LibroRicette;

Magazzino *magazzino;
LibroRicette *ricette;

typedef struct NodoOrdine {
    Ricetta *ricetta;    // Nome dell'ordine (nome del dolce)
    int quantita;                  // Quantità dell'ordine
    int tempo_arrivo;              // Tempo di arrivo dell'ordine
    struct NodoOrdine *prossimo;   // Puntatore all'ordine successivo
    struct NodoOrdine *precedente; // Puntatore all'ordine precedente
} NodoOrdine;

typedef struct CodaOrdini {
    NodoOrdine *testa;
    NodoOrdine *coda;
} CodaOrdini;

typedef struct Camioncino {
    CodaOrdini ordini_caricati;  // Lista doppiamente concatenata degli ordini caricati
    CodaOrdini ordini_da_caricare;  // Lista doppiamente concatenata degli ordini da caricare
    int capienza_massima;          // Capacità massima del camioncino
    int capienza_rimasta;          // Capacità rimanente
} Camioncino;

CodaOrdini ordini_in_attesa = { NULL, NULL };
CodaOrdini *coda_ordini;

// Funzione hash
unsigned long funzione_hash(const char *str, int size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % size;
}

// Funzione per rimuovere il newline alla fine di una stringa
void rimuovi_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len-1] == '\n') {
        str[len-1] = '\0';
    }
}

Magazzino* crea_magazzino(int size) {
    Magazzino *mag = malloc(sizeof(Magazzino));
    mag->size = size;
    mag->count = 0;
    mag->buckets = calloc(mag->size, sizeof(NodoMagazzino*));
    return mag;
}

LibroRicette* crea_libro_ricette(int size) {
    LibroRicette *libro = malloc(sizeof(LibroRicette));
    libro->size = size;
    libro->count = 0;
    libro->buckets = calloc(libro->size, sizeof(Ricetta*));
    return libro;
}

void sottrai_ingredienti(Ricetta *ricetta,  int quantita_ordine) {
    IngredienteRicetta *ingrediente_richiesto = ricetta->ingredienti;

    while (ingrediente_richiesto != NULL) {
        int quantita_totale_necessaria = ingrediente_richiesto->quantita_ricetta * quantita_ordine;
        NodoMagazzino *ingrediente_magazzino = ingrediente_richiesto->riferimento_al_magazzino;

        Lotto *lotto_current = ingrediente_magazzino->lotto;

        // Sottrai la quantità totale necessaria dalla quantità disponibile nel magazzino
        ingrediente_magazzino->quantita_disponibile -= quantita_totale_necessaria;

        while (quantita_totale_necessaria > 0 && lotto_current != NULL) {
            // Assumi che gli ingredienti scaduti siano già stati rimossi da 'utilizza_ingredienti'
            if (lotto_current->quantita <= quantita_totale_necessaria) {
                quantita_totale_necessaria -= lotto_current->quantita;

                // Aggiorna la lista rimuovendo il lotto usato
                ingrediente_magazzino->lotto = lotto_current->prossimo;
                // Libera la memoria del lotto corrente
                free(lotto_current);
                // Passa al prossimo lotto
                lotto_current = ingrediente_magazzino->lotto;
            } else {
                // Sottrai la quantità necessaria dal lotto corrente
                lotto_current->quantita -= quantita_totale_necessaria;
                quantita_totale_necessaria = 0;
            }
        }

        ingrediente_richiesto = ingrediente_richiesto->prossimo;
    }
}

int utilizza_ingredienti(Ricetta *ricetta, int quantita_ordine, int tempo_corrente) {
    IngredienteRicetta *ingrediente_richiesto = ricetta->ingredienti;
    IngredienteRicetta *ingrediente_precedente = NULL;

    while (ingrediente_richiesto != NULL) {
        int quantita_totale_necessaria = ingrediente_richiesto->quantita_ricetta * quantita_ordine;
        NodoMagazzino *ingrediente_magazzino = ingrediente_richiesto->riferimento_al_magazzino;

        Lotto *lotto_current = ingrediente_magazzino->lotto;
        Lotto *lotto_prossimo;

        // Pulisci ingredienti scaduti
        while (lotto_current != NULL) {
            lotto_prossimo = lotto_current->prossimo;

            if (lotto_current->scadenza <= tempo_corrente) {
                // Rimuovi l'ingrediente scaduto
                ingrediente_magazzino->lotto = lotto_prossimo;
                // Aggiorna quantità disponibile
                ingrediente_magazzino->quantita_disponibile -= lotto_current->quantita;
                // Libera la memoria
                free(lotto_current);
            } else {
                break; // Non ci sono più lotti scaduti (sono in ordine)
            }

            lotto_current = lotto_prossimo;
        }

        if (ingrediente_magazzino->quantita_disponibile < quantita_totale_necessaria) {
            // Sposta l'ingrediente insufficiente all'inizio della lista
            if (ingrediente_precedente != NULL) {
                ingrediente_precedente->prossimo = ingrediente_richiesto->prossimo;
                ingrediente_richiesto->prossimo = ricetta->ingredienti;
                ricetta->ingredienti = ingrediente_richiesto;
            }
            return 0; // Non ci sono abbastanza ingredienti non scaduti
        }

        ingrediente_precedente = ingrediente_richiesto;
        ingrediente_richiesto = ingrediente_richiesto->prossimo;
    }

    return 1; // Tutti gli ingredienti necessari sono disponibili
}

// Funzione per aggiungere un ingrediente al magazzino
void aggiungi_ingrediente_magazzino(Magazzino *mag, char *nome, int quantita, int scadenza) {

    // Calcola l'indice nella tabella hash
    int indice = funzione_hash(nome, mag->size);
    // Cerca se esiste gi  un ingrediente con lo stesso nome nel magazzino
    NodoMagazzino *current = mag->buckets[indice];
    while (current) {
    	if (strcmp(current->nome, nome) == 0) {
    		break;
		}
		current = current->prossimo;
	}

	// In caso negativo: crea un nuovo nodo per l'ingrediente
	if (current == NULL) {
	    // Crea un nuovo nodo per l'ingrediente nel magazzino
	    current = (NodoMagazzino*)malloc(sizeof(NodoMagazzino));
	    if (current == NULL) {
	        perror("Impossibile allocare memoria per nuovo nodo di magazzino");
	        return;
	    }

		// Inizializza i suoi valori
	    current->nome = strdup(nome);
	    current->quantita_disponibile = 0;
	    current->lotto = NULL;

	    // Inserisci il nuovo nodo nella lista di nodi nello stesso bucket
	    current->prossimo = mag->buckets[indice];
	    mag->buckets[indice] = current;

	    // Incrementa il contatore degli elementi nel magazzino
	    mag->count++;
	}

	// aggiorna la quantit  totale
	current->quantita_disponibile += quantita;

	// Crea un nuovo lotto per l'ingrediente nel magazzino
	Lotto *nuovo_lotto = (Lotto*)malloc(sizeof(Lotto));
    if (nuovo_lotto == NULL) {
        perror("Impossibile allocare memoria per nuovo lotto");
        return;
    }

    // Inizializza i suoi valori
    nuovo_lotto->quantita = quantita;
    nuovo_lotto->scadenza = scadenza;

    // Inserisci il nuovo ingrediente nella lista mantenendo l'ordinamento per scadenza
    Lotto *lotto_current = current->lotto;
    Lotto *lotto_prev = NULL;

    // Trova la posizione corretta per inserire il nuovo nodo
    while (lotto_current != NULL && lotto_current->scadenza < scadenza) {
        lotto_prev = lotto_current;
        lotto_current = lotto_current->prossimo;
    }

    // Inserisce il nuovo nodo
    if (lotto_prev == NULL) {
        // Inserisce il nuovo nodo all'inizio della lista
        nuovo_lotto->prossimo = current->lotto;
        current->lotto = nuovo_lotto;
    } else {
        // Inserisce il nuovo nodo tra lotto_prev e lotto_current
        nuovo_lotto->prossimo = lotto_current;
        lotto_prev->prossimo = nuovo_lotto;
    }
}

// Funzione per aggiungere una nuova ricetta
void aggiungi_ricetta(LibroRicette *libro, char *nome_ricetta, IngredienteRicetta *ingredienti) {

    // Calcola l'indice nella tabella hash
    int indice = funzione_hash(nome_ricetta, libro->size);

    // Verifica se la ricetta esiste già
    Ricetta *current = libro->buckets[indice];
    while (current != NULL) {
        if (strcmp(current->nome, nome_ricetta) == 0) {
            printf("ignorato\n");
            return;
        }
        current = current->prossimo;
    }

    // Alloca memoria per la nuova ricetta
    Ricetta *nuova_ricetta = (Ricetta *)malloc(sizeof(Ricetta));

    // Copia il nome della ricetta e imposta i campi
    nuova_ricetta->nome = strdup(nome_ricetta);
    nuova_ricetta->ingredienti = ingredienti;
    nuova_ricetta->ordini_in_sospeso = 0;
    nuova_ricetta->prossimo = libro->buckets[indice];

    // Calcola il peso totale della ricetta
    int peso_totale = 0;
    IngredienteRicetta *ingrediente_corrente = ingredienti;
    while (ingrediente_corrente != NULL) {
        peso_totale += ingrediente_corrente->quantita_ricetta;
        ingrediente_corrente = ingrediente_corrente->prossimo;
    }
    nuova_ricetta->peso_totale = peso_totale;

    // Inserisce la nuova ricetta nella lista delle ricette
    libro->buckets[indice] = nuova_ricetta;

    // Incrementa il contatore delle ricette
    libro->count++;

    // Stampa e scrive nel file che la ricetta è stata aggiunta
    printf("aggiunta\n");
}


void rimuovi_ricetta(LibroRicette *libro, char *nome_ricetta) {
    // Cerca la ricetta
    int indice = funzione_hash(nome_ricetta, libro->size);
    Ricetta *current = libro->buckets[indice];
    Ricetta *prev = NULL;
    while (current != NULL) {
    	if (strcmp(current->nome, nome_ricetta) == 0) {
    		break;
		}
		prev = current;
		current = current->prossimo;
	}
    // Se la ricetta non è stata trovata finisci già
    if (current == NULL) {
    	printf("non presente\n");
    	return;
	}

    // Controlla se ci sono ordini in sospeso
    if (current->ordini_in_sospeso > 0) {
    	printf("ordini in sospeso\n");
    	return;
	}

	// Altrimenti: procedi con la rimozione normale

    if (prev == NULL) {
        // Rimuove il nodo all'inizio della lista
        libro->buckets[indice] = current->prossimo;
    } else {
        // Rimuove il nodo non all'inizio della lista
        prev->prossimo = current->prossimo;
    }

    // Libera la memoria degli ingredienti della ricetta
    IngredienteRicetta *ingrediente = current->ingredienti;
    while (ingrediente != NULL) {
        IngredienteRicetta *temp = ingrediente;
        ingrediente = ingrediente->prossimo;
        free(temp);
    }

    // Libera la memoria della ricetta
    free(current->nome);
    free(current);

    // Stampa e scrive nel file che la ricetta è stata rimossa
    printf("rimossa\n");
}

// Trova l'ultimo nodo della lista
NodoOrdine* trova_ultimo_nodo(NodoOrdine *testa) {
    while (testa != NULL && testa->prossimo != NULL) {
        testa = testa->prossimo;
    }
    return testa;
}

// Funzione di partizionamento ottimizzata
NodoOrdine* partiziona(NodoOrdine *testa, NodoOrdine *fine, NodoOrdine **nuova_testa, NodoOrdine **nuova_fine, LibroRicette *libro) {
    NodoOrdine *pivot = fine;
    NodoOrdine *precedente = NULL, *corrente = testa, *coda = pivot;

    while (corrente != pivot) {
        int peso_corrente = corrente->ricetta->peso_totale * corrente->quantita;
        int peso_pivot = pivot->ricetta->peso_totale * pivot->quantita;

        if (peso_corrente > peso_pivot || (peso_corrente == peso_pivot && corrente->tempo_arrivo < pivot->tempo_arrivo)) {
            if ((*nuova_testa) == NULL) {
                (*nuova_testa) = corrente;
            }
            precedente = corrente;
        } else {
            if (precedente) {
                precedente->prossimo = corrente->prossimo;
            }
            NodoOrdine *temp = corrente->prossimo;
            corrente->prossimo = NULL;
            coda->prossimo = corrente;
            coda = corrente;
            corrente = temp;
            continue;
        }
        corrente = corrente->prossimo;
    }

    if ((*nuova_testa) == NULL) {
        (*nuova_testa) = pivot;
    }

    (*nuova_fine) = coda;

    return pivot;
}

// Funzione quicksort ottimizzata
NodoOrdine* quicksort(NodoOrdine *testa, NodoOrdine *fine, LibroRicette *libro) {
    if (!testa || testa == fine) {
        return testa;
    }

    NodoOrdine *nuova_testa = NULL, *nuova_fine = NULL;

    NodoOrdine *pivot = partiziona(testa, fine, &nuova_testa, &nuova_fine, libro);

    if (nuova_testa != pivot) {
        NodoOrdine *temp = nuova_testa;
        while (temp->prossimo != pivot) {
            temp = temp->prossimo;
        }
        temp->prossimo = NULL;

        nuova_testa = quicksort(nuova_testa, temp, libro);

        temp = trova_ultimo_nodo(nuova_testa);
        temp->prossimo = pivot;
    }

    pivot->prossimo = quicksort(pivot->prossimo, nuova_fine, libro);

    return nuova_testa;
}

// Funzione di ordinamento finale
NodoOrdine* ordina_ordini_per_peso_e_tempo(LibroRicette *libro, NodoOrdine* testa) {
    if (!testa) return NULL;

    NodoOrdine *fine = trova_ultimo_nodo(testa);
    return quicksort(testa, fine, libro);
}


// Funzione per stampare la lista degli ordini in ordine corretto
void stampa_lista(NodoOrdine *testa) {
    NodoOrdine *corrente = testa;
    while (corrente != NULL) {
        printf("%d %s %d\n",
               corrente->tempo_arrivo, corrente->ricetta->nome, corrente->quantita);
        corrente = corrente->prossimo;
    }
}

// Funzione per gestire l'arrivo del corriere
void gestisci_arrivo_corriere(LibroRicette *libro, Camioncino *camioncino) {
    NodoOrdine *corrente = camioncino->ordini_da_caricare.testa;

    if (corrente == NULL) {
        printf("camioncino vuoto\n");
        return;
    }

    while (corrente != NULL && camioncino->capienza_rimasta > 0) {
        int peso_ordine = corrente->ricetta->peso_totale * corrente->quantita;

        if (peso_ordine <= camioncino->capienza_rimasta) {
            NodoOrdine *next = corrente->prossimo;

            if (camioncino->ordini_da_caricare.testa == corrente) {
                camioncino->ordini_da_caricare.testa = next;
            } else {
                corrente->precedente->prossimo = next;
            }

            if (next != NULL) {
                next->precedente = corrente->precedente;
            } else {
                camioncino->ordini_da_caricare.coda = corrente->precedente;
            }

            corrente->prossimo = camioncino->ordini_caricati.testa;
            corrente->precedente = NULL;
            if (camioncino->ordini_caricati.testa != NULL) {
                camioncino->ordini_caricati.testa->precedente = corrente;
            }
            camioncino->ordini_caricati.testa = corrente;
            if (camioncino->ordini_caricati.coda == NULL) {
                camioncino->ordini_caricati.coda = corrente;
            }

            camioncino->capienza_rimasta -= peso_ordine;

            corrente = next;
        } else {
            break;
        }
    }

    if (camioncino->ordini_caricati.testa == NULL) {
        printf("camioncino vuoto\n");
    } else {
        camioncino->ordini_caricati.testa = ordina_ordini_per_peso_e_tempo(libro, camioncino->ordini_caricati.testa);
        stampa_lista(camioncino->ordini_caricati.testa);

        corrente = camioncino->ordini_caricati.testa;
        while (corrente != NULL) {
			corrente->ricetta->ordini_in_sospeso--;
            NodoOrdine *next = corrente->prossimo;
            free(corrente);
            corrente = next;
        }

        camioncino->ordini_caricati.testa = NULL;
        camioncino->ordini_caricati.coda = NULL;
        camioncino->capienza_rimasta = camioncino->capienza_massima;
    }
}


void inserisci_ordine(CodaOrdini *lista, NodoOrdine *nuovo_ordine) {
    // Se la lista è vuota, inserisci l'ordine come unico elemento
    if (lista->testa == NULL) {
        lista->testa = nuovo_ordine;
        lista->coda = nuovo_ordine;
        nuovo_ordine->prossimo = NULL;
        nuovo_ordine->precedente = NULL;
        return;
    }

    // Se il nuovo ordine deve essere inserito in fondo alla lista
    if (nuovo_ordine->tempo_arrivo >= lista->coda->tempo_arrivo) {
        nuovo_ordine->precedente = lista->coda;
        nuovo_ordine->prossimo = NULL;
        lista->coda->prossimo = nuovo_ordine;
        lista->coda = nuovo_ordine;
        return;
    }

    // Trova la posizione corretta per inserire il nuovo ordine
    NodoOrdine *corrente = lista->testa;
    NodoOrdine *predecessore = NULL;

    while (corrente != NULL && corrente->tempo_arrivo < nuovo_ordine->tempo_arrivo) {
        predecessore = corrente;
        corrente = corrente->prossimo;
    }

    // Inserisci il nuovo ordine nella posizione trovata
    if (predecessore == NULL) {
        // Inserimento all'inizio della lista
        nuovo_ordine->prossimo = lista->testa;
        nuovo_ordine->precedente = NULL;
        lista->testa->precedente = nuovo_ordine;
        lista->testa = nuovo_ordine;
    } else {
        // Inserimento tra predecessore e corrente
        nuovo_ordine->prossimo = corrente;
        nuovo_ordine->precedente = predecessore;
        predecessore->prossimo = nuovo_ordine;
        if (corrente != NULL) {
            corrente->precedente = nuovo_ordine;
        }
    }

    // Aggiorna la coda se necessario
    if (nuovo_ordine->prossimo == NULL) {
        lista->coda = nuovo_ordine;
    }
}

void inserisci_ordine_in_attesa(NodoOrdine *nuovo_ordine, CodaOrdini *ordini_in_attesa) {
    if (ordini_in_attesa->testa == NULL) {
        // Se la coda è vuota, inserisci come primo elemento
        ordini_in_attesa->testa = nuovo_ordine;
        ordini_in_attesa->coda = nuovo_ordine;
        nuovo_ordine->prossimo = NULL;
        nuovo_ordine->precedente = NULL;
        return;
    }
    // Inserisci alla fine
    nuovo_ordine->prossimo = NULL;
    nuovo_ordine->precedente = ordini_in_attesa->coda;
    ordini_in_attesa->coda->prossimo = nuovo_ordine;
    ordini_in_attesa->coda = nuovo_ordine;
}



// Funzione per processare un ordine
void processa_ordine(char *nome, int quantita, int tempo_arrivo, Camioncino *camioncino, LibroRicette *libro, Magazzino *magazzino) {
    // Calcola l'indice del bucket nella tabella hash del libro di ricette
    int indice_ricetta = funzione_hash(nome, libro->size);
    Ricetta *ricetta = libro->buckets[indice_ricetta];

    // Cerca la ricetta nella lista collegata del bucket
    while (ricetta != NULL && strcmp(ricetta->nome, nome) != 0) {
        ricetta = ricetta->prossimo;
    }

    // Se la ricetta non è trovata, rifiuta l'ordine
    if (ricetta == NULL) {
        printf("rifiutato\n");
        return;
    }

    // L'ordine sarà accettato
    ricetta->ordini_in_sospeso++;

    // Verifica la disponibilità degli ingredienti
    if (utilizza_ingredienti(ricetta, quantita, tempo_arrivo)) {
        // Sottrai gli ingredienti utilizzati
        sottrai_ingredienti(ricetta, quantita);

        // Crea un nuovo ordine e inseriscilo nella coda del camioncino
        NodoOrdine *nuovo_ordine = (NodoOrdine *)malloc(sizeof(NodoOrdine));

        nuovo_ordine->ricetta = ricetta;
        nuovo_ordine->quantita = quantita;
        nuovo_ordine->tempo_arrivo = tempo_arrivo;
        nuovo_ordine->prossimo = NULL;
        nuovo_ordine->precedente = NULL;

        inserisci_ordine(&camioncino->ordini_da_caricare, nuovo_ordine);
    } else {
        // Crea un nuovo ordine e inseriscilo nella lista degli ordini in attesa
        NodoOrdine *nuovo_ordine = (NodoOrdine *)malloc(sizeof(NodoOrdine));

        nuovo_ordine->ricetta = ricetta;
        nuovo_ordine->quantita = quantita;
        nuovo_ordine->tempo_arrivo = tempo_arrivo;
        nuovo_ordine->prossimo = NULL;
        nuovo_ordine->precedente = NULL;

        inserisci_ordine_in_attesa(nuovo_ordine, &ordini_in_attesa);
    }

	printf("accettato\n");
}


// Funzione per leggere il file di input
void leggi_file_input(int periodicita_corriere, int capienza_corriere) {
    char line[1024];
    int tempo = 0;

    // Inizializza il camioncino
    Camioncino camioncino;
    camioncino.ordini_caricati.testa = NULL;
    camioncino.ordini_caricati.coda = NULL;
    camioncino.ordini_da_caricare.testa = NULL;
    camioncino.ordini_da_caricare.coda = NULL;
    camioncino.capienza_massima = capienza_corriere;
    camioncino.capienza_rimasta = capienza_corriere;

    // Leggi dal stdin
    while (fgets(line, sizeof(line), stdin)) {
        rimuovi_newline(line);  // Rimuove il \n dalla riga letta
        char *token = strtok(line, " ");
        if (token == NULL) {
            continue;
        }

        if (strcmp(token, "rifornimento") == 0) {
            int rifornito = 0;
            while ((token = strtok(NULL, " ")) != NULL) {
                char nome_ingrediente[MAX_NAME_LENGTH];
                strcpy(nome_ingrediente, token);
                token = strtok(NULL, " ");
                int quantita = atoi(token);
                token = strtok(NULL, " ");
                int scadenza = atoi(token);

                // Chiamata alla funzione aggiornata di aggiunta ingrediente
                aggiungi_ingrediente_magazzino(magazzino, nome_ingrediente, quantita, scadenza);
                rifornito = 1;
            }

            if (rifornito) {
                printf("rifornito\n");
            }

            // Dopo aver rifornito, controlla gli ordini in attesa
            NodoOrdine *ordine_precedente = NULL;
            NodoOrdine *ordine_attuale = ordini_in_attesa.testa;
            while (ordine_attuale != NULL) {
                if (utilizza_ingredienti(ordine_attuale->ricetta, ordine_attuale->quantita, tempo)) {
                    sottrai_ingredienti(ordine_attuale->ricetta, ordine_attuale->quantita);

                    // Rimuovi l'ordine dalla lista degli ordini in attesa e caricalo sul camioncino
                    if (ordine_precedente == NULL) {
                        ordini_in_attesa.testa = ordine_attuale->prossimo;
                    } else {
                        ordine_precedente->prossimo = ordine_attuale->prossimo;
                    }
                    if (ordine_attuale->prossimo != NULL) {
                        ordine_attuale->prossimo->precedente = ordine_precedente;
                    } else {
                        ordini_in_attesa.coda = ordine_precedente;
                    }

                    NodoOrdine *ordine_completato = ordine_attuale;
                    ordine_attuale = ordine_attuale->prossimo;

                    // Aggiungi l'ordine completato alla lista degli ordini del camioncino
                    inserisci_ordine(&camioncino.ordini_da_caricare, ordine_completato);
                } else {
                    ordine_precedente = ordine_attuale;
                    ordine_attuale = ordine_attuale->prossimo;
                }
            }

        } else if (strcmp(token, "aggiungi_ricetta") == 0) {
            char nome_ricetta[MAX_NAME_LENGTH];
            strcpy(nome_ricetta, strtok(NULL, " "));
            IngredienteRicetta *lista_ingredienti = NULL;
            IngredienteRicetta *ultimo_ingrediente = NULL;

            while ((token = strtok(NULL, " ")) != NULL) {
            	// Crea nuovo ingrediente e associa il corretto riferimento al magazzino
                IngredienteRicetta *nuovo_ingrediente = (IngredienteRicetta*)malloc(sizeof(IngredienteRicetta));

			    // Calcola l'indice nella tabella hash
			    int indice_mag = funzione_hash(token, magazzino->size);
			    // Cerca se esiste gi  un ingrediente con lo stesso nome nel magazzino
			    NodoMagazzino *nodo_mag = magazzino->buckets[indice_mag];
			    while (nodo_mag) {
			    	if (strcmp(nodo_mag->nome, token) == 0) {
			    		break;
					}
					nodo_mag = nodo_mag->prossimo;
				}

				if (nodo_mag != NULL) {
					nuovo_ingrediente->riferimento_al_magazzino = nodo_mag;
				}
				else {
					// Crea un nodo magazzino che conterr  ingredienti del tipo di quello aggiunto a questa ricetta
				    nuovo_ingrediente->riferimento_al_magazzino = (NodoMagazzino*)malloc(sizeof(NodoMagazzino));
				    if (nuovo_ingrediente->riferimento_al_magazzino == NULL) {
				        perror("Impossibile allocare memoria per nuovo nodo di magazzino");
				        return;
				    }

					// Inizializza i suoi valori
				    nuovo_ingrediente->riferimento_al_magazzino->nome = strdup(token);
				    nuovo_ingrediente->riferimento_al_magazzino->quantita_disponibile = 0;
				    nuovo_ingrediente->riferimento_al_magazzino->lotto = NULL;

				    // Inserisci il nuovo nodo nella lista di nodi nello stesso bucket
				    nuovo_ingrediente->riferimento_al_magazzino->prossimo = magazzino->buckets[indice_mag];
				    magazzino->buckets[indice_mag] = nuovo_ingrediente->riferimento_al_magazzino;

				    // Incrementa il contatore degli elementi nel magazzino
				    magazzino->count++;
				}

                // Assegna altri valori
                token = strtok(NULL, " ");
                nuovo_ingrediente->quantita_ricetta = atoi(token);
                nuovo_ingrediente->prossimo = NULL;

				// Aggiungi alla lista
                if (lista_ingredienti == NULL) {
                    lista_ingredienti = nuovo_ingrediente;
                } else {
                    ultimo_ingrediente->prossimo = nuovo_ingrediente;
                }
                ultimo_ingrediente = nuovo_ingrediente;
            }

            // Chiamata alla funzione aggiornata di aggiunta ricetta
            aggiungi_ricetta(ricette, nome_ricetta, lista_ingredienti);

        } else if (strcmp(token, "rimuovi_ricetta") == 0) {
            char nome_ricetta[MAX_NAME_LENGTH];
            strcpy(nome_ricetta, strtok(NULL, " "));
            rimuovi_newline(nome_ricetta);  // Rimuove il \n dal nome della ricetta, se presente

            // Chiamata alla funzione aggiornata di rimozione ricetta
            rimuovi_ricetta(ricette, nome_ricetta);

        } else {
            char nome_ordine[MAX_NAME_LENGTH];
            strcpy(nome_ordine, strtok(NULL, " "));
            int quantita = atoi(strtok(NULL, " "));

            // Processa l'ordine
            processa_ordine(nome_ordine, quantita, tempo, &camioncino, ricette, magazzino);
        }

        // Incrementa il tempo ad ogni linea processata
        tempo++;

        // Gestisci l'arrivo del corriere in base alla periodicità
        if (tempo % periodicita_corriere == 0) {
            gestisci_arrivo_corriere(ricette, &camioncino);
        }
    }
}

int main() {
	//freopen("open8.txt", "r", stdin);
	//freopen("output.txt", "w", stdout);

    int periodicita_corriere, capienza_corriere;

    magazzino = crea_magazzino(INITIAL_TABLE_SIZE);
    ricette = crea_libro_ricette(INITIAL_TABLE_SIZE);

    // Leggi la periodicità e la capienza del corriere dal primo input da stdin
    if (scanf("%d %d", &periodicita_corriere, &capienza_corriere) == 2)
        leggi_file_input(periodicita_corriere, capienza_corriere);

    return 0;
}
