
# Algorithms and Data Structures Project 2023-2024 (PoliMI)

## Project Overview
This constitutes the first of the three final projects for the Bachelor's in Engineering of Computing Systems at Politecnico di Milano.

The project is managed by **Prof. Matteo Pradella**.

The goal is to implement the software for managing a pastry shop (order management, stock management, recipe management, ...) using C, while optimizing time and memory efficciency.

## Development Tools
- Language: C11 (standard library, no multithreading)
- Editor: VSCode
- Testing: GDB, Valgrind, Callgrind, Massif

## Specifications

The entire simulation takes place in discrete time steps. It is assumed that after each received command is executed, one time step passes. The simulation starts at time 0.

Each command is recieved as a string which can be one of the following:

- `aggiungi_ricetta <recipe_name> <ingredient_name> <quantity> ...`
	- Adds a new recipe to the catalog.
	- Output: `aggiunta` or `ignorato`
- `rimuovi_ricetta <recipe_name>`
	- Removes a recipe from the catalog.
	- Output: `rimossa`, `ordini in sospeso`, or `non presente`
- `rifornimento <ingredient_name> <quantity> <expiration> ...`
	- Restocks ingredients with specified quantities and expiration dates.
	- Output: `rifornito`
- `ordine <recipe_name> <quantity>`
	- Places an order for a specified number of pastries.
	- Output: `accettato` or `rifiutato`

In addition to the above outputs, the program prints the orders in the courier's truck as a sequence of triples `⟨time_of_order_arrival⟩ ⟨recipe_name⟩ ⟨number_of_items_ordered⟩`, one per line, in a specific order. The courier’s periodicity and capacity are given at the start of the simulation. The printout is done before handling the orders.

It is assumed that all quantities are integers and greater than zero, regardless of the unit of measurement. Names, whether of ingredients or recipes, are defined using the alphabet {a, ..., z, A, ..., Z, 0, ..., 9, _} and can be up to 255 characters long.

The simulation concludes when all commands have been processed.

**Detailed specifications are available in Italian.**

## Project Status
To be able to pass, the program must run and provide a correct output for a secret remote test. The final grade is determined based on the time and peak memory used during execution of the secret test. Both requirements must be met to achieve the grade.
|Grade|Time|Memory|Passed|
|--|--|--|--|
|OPEN|45.0 s|150.0 MiB|✅
|18| 14.0 s|35.0 MiB|✅
|21|11.5 s|30.0 MiB|✅
|24|9.0 s|25.0 MiB|✅
|27|6.5 s|20.0 MiB|✅
|30|4.0 s|15.0 MiB|✅
|30L|1.5 s|14.0 MiB|✅

**Final grade 30 cum laude.**

A set of public tests and expected outputs are also provided to test the program's functionality and efficiency.
