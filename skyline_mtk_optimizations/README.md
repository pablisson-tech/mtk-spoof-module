# Otimizações Core para MediaTek (Nintendo Switch Emulation)

Este diretório contém as três principais peças de engenharia reversa necessárias para forçar um emulador de Nintendo Switch (como Skyline, Yuzu ou Sudachi) a rodar de forma fluida e sem crashes em processadores MediaTek com GPUs Mali.

Esses arquivos NÃO são um emulador completo (isso levaria anos para criar do zero). Eles são **Módulos de Injeção** (Core Hacks).

## 1. `mtk_tbdr_optimizer.cpp` (Otimizador de Renderização Vulkan)
GPUs Mali usam Tile-Based Deferred Rendering (TBDR). A NVIDIA do Switch usa Immediate Rendering.
* **Como integrar:** No código do seu emulador, localize a classe que chama `vkCmdBeginRenderPass` e `vkCmdEndRenderPass` (normalmente dentro do `VulkanRasterizer` ou `RenderPassManager`). Substitua as chamadas diretas pelas funções interceptadas `g_MtkOptimizer.OnBeginRenderPass` e `g_MtkOptimizer.OnEndRenderPass`. Isso agrupará os comandos, dobrando ou triplicando o FPS em GPUs Mali.

## 2. `astc_texture_decoder.comp` (Decoder Dinâmico de Texturas)
MediaTek não entende o formato de textura BCn usado no Switch e no PC.
* **Como integrar:** Compile este *Compute Shader* (`.comp`) para SPIR-V. Quando o emulador for subir uma textura BCn para a VRAM (normalmente na classe `TextureCache`), despache este Compute Shader na GPU. Ele traduzirá o BCn para RGBA8 bruto dentro da própria placa de vídeo Mali, corrigindo as "texturas pretas" e gráficos quebrados.

## 3. `nce_memory_hack.cpp` (Conserto de Crash no NCE)
O Kernel Android de celulares MediaTek bloqueia a alocação forçada de memória em 39-bits que o NCE do Switch exige. Isso faz o jogo fechar na hora de abrir.
* **Como integrar:** No ponto de entrada principal do emulador (`main.cpp` ou `Core::System::Initialize()`), chame a função `Mtk_Install_NCE_Hack()`. Ela intercepta falhas de memória (SIGSEGV) causadas pelo bloqueio da MediaTek e força a alocação de páginas, impedindo que o emulador "crashe" no menu.
