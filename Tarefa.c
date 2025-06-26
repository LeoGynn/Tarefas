#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Estrutura para representar uma tarefa
typedef struct Task {
    int id;           // ID único da tarefa
    char *description; // Descrição da tarefa (alocada dinamicamente)
    bool completed;   // Indica se a tarefa está concluída (true) ou pendente (false)
    struct Task *next; // Ponteiro para a próxima tarefa na lista ligada
} Task;

// Estrutura para a lista ligada de tarefas
typedef struct {
    Task *head; // Ponteiro para o primeiro nó da lista
    Task *tail; // Ponteiro para o último nó da lista (para inserção O(1) no final)
    int next_id; // Próximo ID disponível para uma nova tarefa
} TaskList;


// Tipos de ações que podem ser desfeitas
typedef enum {
    ACTION_ADD,
    ACTION_COMPLETE,
    ACTION_REMOVE
} ActionType;

// Estrutura para armazenar informações de uma ação
typedef struct Action {
    ActionType type;       // Tipo da ação
    int task_id;           // ID da tarefa envolvida na ação
    char *task_description; // Descrição da tarefa (para REMOVE e ADD)
    bool was_completed;     // Estado anterior da tarefa (para COMPLETE)
    struct Action *next;   // Ponteiro para a próxima ação na pilha
} Action;

// Estrutura para a pilha de ações (histórico)
typedef struct {
    Action *top; // Ponteiro para o topo da pilha
} ActionStack;

// --- Funções da Lista de Tarefas ---

// Inicializa a lista de tarefas
void initTaskList(TaskList *list) {
    list->head = NULL;
    list->tail = NULL;
    list->next_id = 1; // Começa os IDs das tarefas a partir de 1
}

// Aloca e cria uma nova tarefa
Task *createTask(int id, const char *description) {
    Task *newTask = (Task *)malloc(sizeof(Task));
    if (!newTask) {
        perror("Erro ao alocar memória para a tarefa");
        exit(EXIT_FAILURE);
    }
    newTask->id = id;
    newTask->description = strdup(description); // Alocação dinâmica para a descrição
    if (!newTask->description) {
        perror("Erro ao alocar memória para a descrição da tarefa");
        free(newTask);
        exit(EXIT_FAILURE);
    }
    newTask->completed = false; // Tarefa inicialmente não concluída
    newTask->next = NULL;
    return newTask;
}

// Adiciona uma tarefa ao final da lista
void addTask(TaskList *list, const char *description) {
    Task *newTask = createTask(list->next_id++, description);
    if (list->head == NULL) {
        list->head = newTask;
        list->tail = newTask;
    } else {
        list->tail->next = newTask;
        list->tail = newTask;
    }
    printf("Tarefa '%s' (ID: %d) adicionada com sucesso.\n", description, newTask->id);
}

// Lista todas as tarefas na lista
void listTasks(const TaskList *list) {
    if (list->head == NULL) {
        printf("Nenhuma tarefa na lista.\n");
        return;
    }
    printf("\n--- Lista de Tarefas ---\n");
    Task *current = list->head;
    while (current != NULL) {
        printf("ID: %d | Estado: [%s] | Descrição: %s\n",
               current->id,
               current->completed ? "X" : " ", // 'X' para concluída, ' ' para pendente
               current->description);
        current = current->next;
    }
    printf("------------------------\n");
}

// Marca uma tarefa como concluída
bool completeTask(TaskList *list, int id) {
    Task *current = list->head;
    while (current != NULL) {
        if (current->id == id) {
            if (current->completed) {
                printf("Tarefa %d já está concluída.\n", id);
                return false;
            }
            current->completed = true;
            printf("Tarefa %d marcada como concluída.\n", id);
            return true;
        }
        current = current->next;
    }
    printf("Tarefa com ID %d não encontrada.\n", id);
    return false;
}

// Remove uma tarefa da lista
// Retorna a tarefa removida (para fins de "desfazer") ou NULL se não encontrada
Task *removeTask(TaskList *list, int id) {
    Task *current = list->head;
    Task *previous = NULL;

    while (current != NULL && current->id != id) {
        previous = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Tarefa com ID %d não encontrada.\n", id);
        return NULL; // Tarefa não encontrada
    }

    // Remover a tarefa se:
    if (previous == NULL) { // Se for o primeiro nó
        list->head = current->next;
    } else {
        previous->next = current->next;
    }

    if (current == list->tail) { // Se for o último nó
        list->tail = previous;
    }

    printf("Tarefa %d ('%s') removida com sucesso.\n", current->id, current->description);
    return current; // Retorna o nó (ainda com memória alocada para ele e sua descrição)
}

// Libera toda a memória da lista de tarefas
void destroyTaskList(TaskList *list) {
    Task *current = list->head;
    while (current != NULL) {
        Task *next = current->next;
        free(current->description); // Libera a descrição
        free(current);             // Libera o nó da tarefa
        current = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->next_id = 1;
    printf("Toda a memória da lista de tarefas foi liberada.\n");
}

// --- Funções da Pilha de Ações (Histórico - para Desfazer) ---

// Inicializa a pilha de ações
void initActionStack(ActionStack *stack) {
    stack->top = NULL;
}

// Empilha uma ação
void pushAction(ActionStack *stack, ActionType type, int task_id, const char *task_description, bool was_completed) {
    Action *newAction = (Action *)malloc(sizeof(Action));
    if (!newAction) {
        perror("Erro ao alocar memória para a ação na pilha");
        exit(EXIT_FAILURE);
    }
    newAction->type = type;
    newAction->task_id = task_id;
    newAction->task_description = NULL; // Inicializa como NULL
    if (task_description) {
        newAction->task_description = strdup(task_description);
        if (!newAction->task_description) {
            perror("Erro ao alocar memória para descrição da ação");
            free(newAction);
            exit(EXIT_FAILURE);
        }
    }
    newAction->was_completed = was_completed;
    newAction->next = stack->top;
    stack->top = newAction;
}

// Desempilha uma ação
Action *popAction(ActionStack *stack) {
    if (stack->top == NULL) {
        return NULL; // Pilha vazia
    }
    Action *action = stack->top;
    stack->top = stack->top->next;
    return action;
}

// Libera toda a memória da pilha de ações
void destroyActionStack(ActionStack *stack) {
    Action *current = stack->top;
    while (current != NULL) {
        Action *next = current->next;
        free(current->task_description); // Libera a descrição, se houver
        free(current);                   // Libera o nó da ação
        current = next;
    }
    stack->top = NULL;
    printf("Toda a memória do histórico de ações foi liberada.\n");
}

// --- Função Desfazer ---
void undoLastAction(TaskList *taskList, ActionStack *actionStack) {
    Action *lastAction = popAction(actionStack);
    if (!lastAction) {
        printf("Nada para desfazer.\n");
        return;
    }

    printf("Desfazendo a última ação...\n");

    switch (lastAction->type) {
        case ACTION_ADD: {
            // Se a última ação foi ADICIONAR, remove a tarefa que foi adicionada.
            // Encontrar a tarefa na lista e removê-la.
            Task *current = taskList->head;
            Task *previous = NULL;
            while (current != NULL && current->id != lastAction->task_id) {
                previous = current;
                current = current->next;
            }
            if (current != NULL) {
                if (previous == NULL) {
                    taskList->head = current->next;
                } else {
                    previous->next = current->next;
                }
                if (current == taskList->tail) {
                    taskList->tail = previous;
                }
                free(current->description);
                free(current);
                printf("Desfeito: Tarefa (ID: %d) removida (originalmente adicionada).\n", lastAction->task_id);
            } else {
                printf("Erro ao desfazer: Tarefa adicionada (ID: %d) não encontrada para remoção.\n", lastAction->task_id);
            }
            break;
        }
        case ACTION_COMPLETE: {
            // Se a última ação foi CONCLUIR, reverte o estado de conclusão da tarefa.
            Task *current = taskList->head;
            while (current != NULL && current->id != lastAction->task_id) {
                current = current->next;
            }
            if (current != NULL) {
                current->completed = lastAction->was_completed;
                printf("Desfeito: Tarefa (ID: %d) estado revertido para %s.\n", lastAction->task_id, lastAction->was_completed ? "concluída" : "pendente");
            } else {
                printf("Erro ao desfazer: Tarefa concluída (ID: %d) não encontrada para reverter.\n", lastAction->task_id);
            }
            break;
        }
        case ACTION_REMOVE: {
            // Se a última ação foi REMOVER, adiciona a tarefa de volta.
            // O ID original da tarefa pode precisar ser ajustado se o next_id do TaskList
            Task *readdedTask = createTask(lastAction->task_id, lastAction->task_description);
            readdedTask->completed = lastAction->was_completed; // Reverte o estado original

            // Adiciona a tarefa de volta na lista.
            if (taskList->head == NULL) {
                taskList->head = readdedTask;
                taskList->tail = readdedTask;
            } else {
                taskList->tail->next = readdedTask;
                taskList->tail = readdedTask;
            }
            // Assegura que o next_id não seja menor que um ID já usado
            if (taskList->next_id <= lastAction->task_id) {
                 taskList->next_id = lastAction->task_id + 1;
            }
            printf("Desfeito: Tarefa '%s' (ID: %d) adicionada novamente.\n", readdedTask->description, readdedTask->id);
            break;
        }
    }

    free(lastAction->task_description); // Libera a descrição da ação
    free(lastAction);                   // Libera o nó da ação
}


// --- Função Principal (main) ---
int main() {
    TaskList myTasks;
    ActionStack undoStack;

    initTaskList(&myTasks);
    initActionStack(&undoStack);

    int choice;
    char description[256]; // Buffer para a descrição da tarefa
    int id_to_process;

    do {
        printf("\n--- Gerenciador de Tarefas ---\n");
        printf("1. Adicionar Tarefa\n");
        printf("2. Listar Tarefas\n");
        printf("3. Marcar Tarefa como Concluída\n");
        printf("4. Remover Tarefa\n");
        printf("5. Desfazer Última Ação\n");
        printf("0. Sair\n");
        printf("Escolha uma opção: ");
        
        // Garante que a entrada numérica seja lida corretamente e limpa o buffer do teclado
        if (scanf("%d", &choice) != 1) {
            printf("Entrada inválida. Por favor, digite um número.\n");
            while (getchar() != '\n'); // Limpa o buffer de entrada
            continue;
        }
        while (getchar() != '\n'); // Limpa o buffer de entrada após a leitura do número

        switch (choice) {
            case 1:
                printf("Digite a descrição da tarefa: ");
                if (fgets(description, sizeof(description), stdin) != NULL) {
                    description[strcspn(description, "\n")] = 0; // Remove o newline
                    addTask(&myTasks, description);
                    // Empilha a ação de ADICIONAR para desfazer
                    pushAction(&undoStack, ACTION_ADD, myTasks.next_id -1, description, false);
                } else {
                    printf("Erro ao ler a descrição.\n");
                }
                break;
            case 2:
                listTasks(&myTasks);
                break;
            case 3:
                printf("Digite o ID da tarefa a ser concluída: ");
                if (scanf("%d", &id_to_process) != 1) {
                    printf("Entrada inválida. Por favor, digite um número.\n");
                    while (getchar() != '\n');
                    break;
                }
                while (getchar() != '\n'); // Limpa o buffer

                Task *taskToComplete = myTasks.head;
                bool was_completed_before = false;
                while(taskToComplete != NULL && taskToComplete->id != id_to_process) {
                    taskToComplete = taskToComplete->next;
                }
                if (taskToComplete != NULL) {
                    was_completed_before = taskToComplete->completed;
                }
                
                if (completeTask(&myTasks, id_to_process)) {
                    // Empilha a ação de CONCLUIR para desfazer
                    pushAction(&undoStack, ACTION_COMPLETE, id_to_process, NULL, was_completed_before);
                }
                break;
            case 4:
                printf("Digite o ID da tarefa a ser removida: ");
                if (scanf("%d", &id_to_process) != 1) {
                    printf("Entrada inválida. Por favor, digite um número.\n");
                    while (getchar() != '\n');
                    break;
                }
                while (getchar() != '\n'); // Limpa o buffer

                Task *removed = removeTask(&myTasks, id_to_process);
                if (removed != NULL) {
                    // Empilha a ação de REMOVER para desfazer
                    pushAction(&undoStack, ACTION_REMOVE, removed->id, removed->description, removed->completed);
                    // Libere a memória da tarefa removida
                    free(removed->description);
                    free(removed);
                }
                break;
            case 5:
                undoLastAction(&myTasks, &undoStack);
                break;
            case 0:
                printf("Saindo do Gerenciador de Tarefas. Até mais!\n");
                break;
            default:
                printf("Opção inválida. Por favor, tente novamente.\n");
        }
    } while (choice != 0);

    // Libera toda a memória
    destroyTaskList(&myTasks);
    destroyActionStack(&undoStack);

    return 0;
}
