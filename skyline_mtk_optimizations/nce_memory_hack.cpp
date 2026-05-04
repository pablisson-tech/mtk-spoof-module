/**
 * MediaTek NCE (Native Code Execution) Memory Hacker
 * 
 * Processadores MediaTek baseados em Cortex (como Dimensity e Helio)
 * sofrem de limites estritos em kernels Android customizados (ASLR e Page Tables limitadas).
 * O Nintendo Switch aloca endereços fixos na faixa de 39-bits de memória.
 * 
 * Quando o NCE falha em alocar esses endereços na MediaTek, o jogo crasha na tela inicial.
 * Solução: Capturamos os Page Faults (SIGSEGV) e fazemos um Fallback de Mapeamento.
 */

#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <android/log.h>

#define LOG_TAG "MTK_NCE_HACK"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static struct sigaction old_sigsegv_action;

// Endereço base de 39-bits exigido pelo Switch (ex: 0x0000008000000000)
const uintptr_t SWITCH_MEMORY_BASE = 0x8000000000;

/**
 * Tratador de Sinal customizado
 * Acionado toda vez que o NCE tenta acessar uma memória inválida.
 */
void Mtk_SigSegv_Handler(int sig, siginfo_t* info, void* context) {
    void* fault_addr = info->si_addr;
    uintptr_t addr = (uintptr_t)fault_addr;

    // Se o emulador tentou acessar a área de 39-bits do Switch e a MediaTek negou:
    if (addr >= SWITCH_MEMORY_BASE && addr < (SWITCH_MEMORY_BASE + 0x100000000)) { // 4GB range
        
        // Alinhamento de página (4KB / 16KB dependendo do Kernel MediaTek)
        size_t page_size = sysconf(_SC_PAGESIZE);
        void* aligned_addr = (void*)(addr & ~(page_size - 1));

        // Tenta forçar o mapeamento ANONYMOUS sobrepondo o ASLR restrito da MediaTek
        void* mapped = mmap(aligned_addr, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, 
                            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

        if (mapped == MAP_FAILED) {
            LOGE("MediaTek Kernel bloqueou mmap fixo no endereço %p", aligned_addr);
            // Acionar rotina de fallback de emulação de software do Skyline (JIT lento, mas não crasha)
            // EmulatorCore::TriggerSoftFallback(addr);
            return;
        }

        // Se o mapeamento teve sucesso, preenchemos com 0 (nop/zeropage) e continuamos a execução NCE!
        memset(mapped, 0, page_size);
        return; 
    }

    // Se não for relacionado ao Switch, repassa para o handler padrão (Crash real)
    if (old_sigsegv_action.sa_flags & SA_SIGINFO) {
        old_sigsegv_action.sa_sigaction(sig, info, context);
    } else if (old_sigsegv_action.sa_handler == SIG_DFL) {
        signal(sig, SIG_DFL);
    } else if (old_sigsegv_action.sa_handler != SIG_IGN) {
        old_sigsegv_action.sa_handler(sig);
    }
}

/**
 * Instala o hack na inicialização do emulador.
 */
void Mtk_Install_NCE_Hack() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = Mtk_SigSegv_Handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

    // Substitui o handler de SEGFAULT do Android
    sigaction(SIGSEGV, &sa, &old_sigsegv_action);
}
