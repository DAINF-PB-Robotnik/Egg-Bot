import { test, expect } from '@playwright/test';

test('editor: desenhar, desfazer, refazer e recuperar rascunho', async ({ page }) => {
  const errors = []; page.on('pageerror', error => errors.push(error.message));
  await page.goto('/');
  await expect(page.getByRole('heading', { name: 'Dê uma volta na criatividade.' })).toBeVisible();
  await page.getByRole('button', { name: 'Novo desenho', exact: true }).click();
  await page.getByRole('button', { name: 'Confirmar', exact: true }).click();
  await expect(page.getByText('O espaço é seu.')).toBeVisible();
  await page.getByLabel('Nome do desenho').fill('Meu desenho');
  const bounds = await page.getByLabel('Área de desenho livre', { exact: true }).boundingBox();
  await page.mouse.move(bounds.x + 40, bounds.y + 80); await page.mouse.down();
  await page.mouse.move(bounds.x + 190, bounds.y + 150, { steps: 12 }); await page.mouse.up();
  await expect(page.getByText('O espaço é seu.')).toHaveCount(0);
  await page.getByRole('button', { name: 'Desfazer', exact: true }).click();
  await expect(page.getByText('O espaço é seu.')).toBeVisible();
  await page.getByRole('button', { name: 'Refazer', exact: true }).click();
  await expect(page.getByText('O espaço é seu.')).toHaveCount(0);
  await expect(page.getByText('Salvo neste navegador')).toBeVisible();
  await page.reload();
  await expect(page.getByLabel('Nome do desenho')).toHaveValue('Meu desenho');
  await expect(page.getByText('O espaço é seu.')).toHaveCount(0);
  expect(errors).toEqual([]);
});

test('trabalho: exportar, enviar, iniciar, pausar, retomar e cancelar', async ({ page }) => {
  await page.goto('/');
  await page.getByRole('button', { name: 'Adicionar padrão Espiral', exact: true }).click();
  const downloadEvent = page.waitForEvent('download');
  await page.getByRole('button', { name: 'Baixar desenho', exact: true }).click();
  expect((await downloadEvent).suggestedFilename()).toMatch(/\.egg$/);
  await page.getByRole('button', { name: 'Enviar ao simulador', exact: true }).click();
  await expect(page.getByRole('heading', { name: 'Tudo pronto para começar' })).toBeVisible();
  await page.getByRole('button', { name: 'Iniciar impressão', exact: true }).click();
  await expect(page.getByRole('heading', { name: 'Seu desenho está ganhando forma' })).toBeVisible();
  await page.getByRole('button', { name: 'Pausar', exact: true }).click();
  await expect(page.getByRole('heading', { name: 'Uma pausa no traço' })).toBeVisible();
  await page.getByRole('button', { name: 'Continuar', exact: true }).click();
  await expect(page.getByRole('heading', { name: 'Seu desenho está ganhando forma' })).toBeVisible();
  await page.getByRole('button', { name: 'Cancelar', exact: true }).click();
  await page.getByRole('button', { name: 'Confirmar', exact: true }).click();
  await expect(page.getByRole('button', { name: 'Iniciar impressão', exact: true })).toBeVisible({ timeout: 15000 });
});

test('SVG é importado sem executar atributos ativos e arquivo mantém contornos', async ({ page }) => {
  await page.goto('/');
  await page.getByRole('button', { name: 'Novo desenho', exact: true }).click();
  await page.getByRole('button', { name: 'Confirmar', exact: true }).click();
  await page.getByLabel('Arquivo SVG', { exact: true }).setInputFiles({ name: 'circulo.svg', mimeType: 'image/svg+xml', buffer: Buffer.from('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" onload="window.importedUnsafe=true"><circle cx="50" cy="50" r="30"/><script>window.importedUnsafe=true</script></svg>') });
  await expect(page.getByLabel('Nome do desenho')).toHaveValue('circulo');
  expect(await page.evaluate(() => window.importedUnsafe)).toBeUndefined();
  await page.getByRole('button', { name: 'Ver arquivo', exact: true }).click();
  await expect(page.locator('.file-preview')).toContainText('EGGJOB 1\nN circulo');
  await expect(page.locator('.file-preview')).toContainText('\nD\n');
});

test('configuração: salvar velocidades e enviar um comando manual', async ({ page }) => {
  await page.goto('/');
  await page.getByRole('button', { name: 'Configuração', exact: true }).click();
  await page.getByLabel('Desenhando', { exact: true }).fill('450');
  await page.getByRole('button', { name: 'Salvar', exact: true }).click();
  await expect(page.getByText('Configuração salva.', { exact: true })).toBeVisible();
  await expect(page.getByLabel('Desenhando', { exact: true })).toHaveValue('450');
  await page.getByRole('button', { name: 'Verificar conexão', exact: true }).click();
  await expect(page.getByText('Verificação solicitada.', { exact: true })).toBeVisible();
  await page.getByText('Console de diagnóstico', { exact: true }).click();
  await expect(page.getByRole('log')).toContainText('EBBv13');
});

test('celular: navegação, controles e largura sem rolagem horizontal', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto('/');
  await expect(page.getByRole('button', { name: 'Enviar ao simulador', exact: true })).toBeEnabled();
  expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
  await page.getByRole('button', { name: 'Abrir navegação', exact: true }).click();
  await page.getByRole('button', { name: 'Configuração', exact: true }).click();
  await expect(page.getByRole('heading', { name: 'O toque da caneta' })).toBeVisible();
  expect(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth)).toBe(true);
  await page.getByRole('button', { name: 'Abrir navegação', exact: true }).click();
  await page.getByRole('button', { name: 'Imprimir', exact: true }).click();
  await expect(page.getByRole('button', { name: 'Abrir estúdio', exact: true })).toBeVisible();
});
