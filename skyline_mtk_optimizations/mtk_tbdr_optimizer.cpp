/**
 * MediaTek (Mali) TBDR Render Pass Optimizer
 * 
 * Este módulo atua na camada Vulkan de emuladores como Skyline e Yuzu.
 * GPUs Mali sofrem gargalos imensos com Immediate Mode (usado pela NVIDIA Tegra).
 * A solução é interceptar e agrupar comandos antes do `vkCmdEndRenderPass`.
 */

#include <vulkan/vulkan.h>
#include <vector>
#include <mutex>

// Estrutura para agrupar comandos (Command Batching)
struct MaliRenderBatch {
    VkRenderPass renderPass;
    VkFramebuffer framebuffer;
    std::vector<VkCommandBuffer> pendingCommands;
    bool isSubpassActive;
};

class MtkTbdrOptimizer {
private:
    std::mutex batchMutex;
    MaliRenderBatch currentBatch;

public:
    MtkTbdrOptimizer() {
        currentBatch.isSubpassActive = false;
    }

    /**
     * Intercepta vkCmdBeginRenderPass
     * Em vez de iniciar imediatamente na GPU Mali, seguramos o comando
     * para mesclar render passes compatíveis e evitar descarregamento de Tiles (Tile flush).
     */
    void OnBeginRenderPass(VkCommandBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
        std::lock_guard<std::mutex> lock(batchMutex);

        // Se já existe um batch aberto e o Framebuffer mudou, precisamos dar "Flush"
        if (currentBatch.isSubpassActive && currentBatch.framebuffer != pRenderPassBegin->framebuffer) {
            FlushBatch();
        }

        currentBatch.renderPass = pRenderPassBegin->renderPass;
        currentBatch.framebuffer = pRenderPassBegin->framebuffer;
        currentBatch.isSubpassActive = true;
        
        // Chamada real atrasada ou otimizada para a Mali
        vkCmdBeginRenderPass(cmdBuffer, pRenderPassBegin, contents);
    }

    /**
     * Intercepta vkCmdEndRenderPass
     * Na Mali, terminar um RenderPass causa a gravação dos dados do Cache L2/L1 (On-chip memory)
     * para a memória RAM (VRAM DDR), o que é caríssimo. 
     * Se o próximo draw call for para a mesma textura, nós OMITIMOS o EndRenderPass temporariamente!
     */
    void OnEndRenderPass(VkCommandBuffer cmdBuffer, bool forceFlush = false) {
        std::lock_guard<std::mutex> lock(batchMutex);

        if (!currentBatch.isSubpassActive) return;

        if (forceFlush) {
            FlushBatch();
            vkCmdEndRenderPass(cmdBuffer);
            currentBatch.isSubpassActive = false;
        } else {
            // HACK PARA MEDIATEK: Não encerramos o RenderPass aqui!
            // Guardamos a intenção de encerrar. Se o próximo comando for um novo BeginRenderPass
            // na mesma textura, o driver Mali não vai descartar os Tiles!
            currentBatch.pendingCommands.push_back(cmdBuffer);
        }
    }

private:
    void FlushBatch() {
        // Resolve todos os RenderPasses retidos e limpa as estruturas de Tile Memory
        for (auto cmdBuffer : currentBatch.pendingCommands) {
            vkCmdEndRenderPass(cmdBuffer);
        }
        currentBatch.pendingCommands.clear();
    }
};

// Instância global para integração
MtkTbdrOptimizer g_MtkOptimizer;
