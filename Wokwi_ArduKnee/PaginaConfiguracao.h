#ifndef PAGINA_CONFIGURACAO_H
#define PAGINA_CONFIGURACAO_H

#include <Arduino.h>

// Pagina servida em "/" - para onde o Portal Cativo redireciona o celular.
const char PAGINA_CONFIGURACAO[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ArduKnee - Configuração</title>
<style>
  body { font-family: Arial, sans-serif; background:#f2f4f7; margin:0; padding:24px; }
  .card { max-width:400px; margin:0 auto; background:#fff; border-radius:12px; padding:24px; box-shadow:0 2px 8px rgba(0,0,0,.1); }
  h1 { font-size:20px; color:#2b6cb0; margin-top:0; }
  label { display:block; margin:16px 0 4px; font-size:14px; color:#333; }
  input, select { width:100%; padding:10px; border:1px solid #ccc; border-radius:8px; box-sizing:border-box; font-size:16px; }
  button { width:100%; margin-top:24px; padding:12px; background:#2b6cb0; color:#fff; border:none; border-radius:8px; font-size:16px; }
  #msg { margin-top:16px; font-size:14px; text-align:center; }
  .aviso { margin-top:20px; font-size:12px; color:#666; text-align:center; }
</style>
</head>
<body>
  <div class="card">
    <h1>ArduKnee - Configurar sessão</h1>
    <form id="formConfig">
      <label>Perfil clínico</label>
      <select id="perfil">
        <option value="1">P1</option>
        <option value="2">P2</option>
        <option value="3">P3</option>
        <option value="4">P4</option>
        <option value="5">P5</option>
        <option value="6">P6</option>
      </select>

      <label>Tipo de exercício</label>
      <select id="tipoExercicio">
        <option value="1">E1 - Flexão</option>
        <option value="2">E2 - Extensão</option>
        <option value="3">E3 - Ciclo completo</option>
      </select>

      <label>Ângulo inicial (0 a 90)</label>
      <input type="number" id="anguloInicial" min="0" max="90" value="10" required>

      <label>Ângulo alvo (0 a 90)</label>
      <input type="number" id="anguloMaximo" min="0" max="90" value="70" required>

      <label>Número de repetições (1 a 100)</label>
      <input type="number" id="repeticoes" min="1" max="100" value="10" required>

      <button type="submit">Salvar configuração</button>
    </form>
    <div id="msg"></div>
    <p class="aviso">Protótipo educacional. Os limites de ângulo e de repetições devem ser definidos com um fisioterapeuta.</p>
  </div>

<script>
document.getElementById('formConfig').addEventListener('submit', async function (e) {
  e.preventDefault();
  const msg = document.getElementById('msg');
  msg.style.color = '#333';
  msg.textContent = 'Enviando...';

  const params = new URLSearchParams();
  params.append('anguloInicial', document.getElementById('anguloInicial').value);
  params.append('anguloMaximo', document.getElementById('anguloMaximo').value);
  params.append('repeticoes', document.getElementById('repeticoes').value);
  params.append('perfil', document.getElementById('perfil').value);
  params.append('tipoExercicio', document.getElementById('tipoExercicio').value);

  try {
    const resp = await fetch('/configurar', { method: 'POST', body: params });
    const data = await resp.json();
    msg.style.color = data.status === 'sucesso' ? 'green' : 'red';
    msg.textContent = data.mensagem;
  } catch (err) {
    msg.style.color = 'red';
    msg.textContent = 'Erro de conexão. Tente novamente.';
  }
});
</script>
</body>
</html>
)rawliteral";

#endif
