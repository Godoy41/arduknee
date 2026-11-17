#ifndef LOG_H
#define LOG_H

// Log de depuracao NAO BLOQUEANTE na mesma serial do CSV.
// Toda linha comeca com "# " : um leitor de CSV (pandas, Excel) trata como comentario,
// entao o log nunca corrompe os dados. Se o buffer da serial nao tem espaco, a linha e'
// DESCARTADA (RF34 / RNF21) - nunca bloqueia o loop().
void logInfo(const char* formato, ...);

#endif
