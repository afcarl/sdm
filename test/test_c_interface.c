#include <sdm/sdm_c.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#define NUM_BAGS 7
#define PTS_PER_BAG 3
#define DIM 2

int main() {
    int i, j;

    sdm_set_log_level(logWARNING);

    double ** d = (double **) malloc(NUM_BAGS * sizeof(double *));
    for (i = 0; i < NUM_BAGS; i++)
        d[i] = malloc(DIM * PTS_PER_BAG * sizeof(double));

    d[0][0] =  0.;   d[0][1] = 0.1 ;
    d[0][2] =  0.01; d[0][3] = 0.89;
    d[0][4] = -0.2;  d[0][5] = 0.95;

    d[1][0] =  0.05; d[1][1] = 0.2 ;
    d[1][2] =  0.02; d[1][3] = 0.90;
    d[1][4] = -0.3;  d[1][5] = 0.96;

    d[2][0] =  4. ;  d[2][1] = 0.2 ;
    d[2][2] =  5. ;  d[2][3] = 0.96;
    d[2][4] =  4.6;  d[2][5] = 0.99;

    d[3][0] =  5. ;  d[3][1] = 0.1 ;
    d[3][2] =  4. ;  d[3][3] = 0.95;
    d[3][4] =  4.5;  d[3][5] = 0.98;

    d[4][0] =  4. ;  d[4][1] = 0.1 ;
    d[4][2] =  4. ;  d[4][3] = 0.95;
    d[4][4] =  3.5;  d[4][5] = 0.98;

    d[5][0] =  4.2;  d[5][1] = 0.12;
    d[5][2] =  4.1;  d[5][3] = 0.94;
    d[5][4] =  3.6;  d[5][5] = 0.99;

    d[6][0] =  0.03; d[6][1] = 0.21;
    d[6][2] =  0.03; d[6][3] = 0.92;
    d[6][4] = -0.31; d[6][5] = 0.94;

    const double ** data = (const double **) d;

    size_t rows[NUM_BAGS] = { 3, 3, 3, 3, 3, 3, 3 };
    int labels[NUM_BAGS] = { 0, 0, 1, 1, 2, 1, 0};
    double means[NUM_BAGS] = { 0.29, 0.305, 2.625, 2.589, 2.255, 2.325, 0.303};

    struct FLANNParameters flann_params = DEFAULT_FLANN_PARAMETERS;
    flann_params.algorithm = FLANN_INDEX_KDTREE_SINGLE;

    DivParamsC div_params = {
        1, // k
        flann_params,
        1, // num_threads
        3, // how often to print progress
        NULL // print_progress_to_stderr
    };

    double acc = sdm_crossvalidate_classify_double(
            data, NUM_BAGS, rows, DIM, labels,
            "renyi:.9", "gaussian",
            &div_params, 2, 0, 1, 1,
            default_c_vals, num_default_c_vals, &default_svm_params, 2);
    printf("CV acc: %g\n", acc);

    printf("\n\nTraining SDM\n");
    SDM_ClassifyD *sdm = SDM_ClassifyD_train(
            data, NUM_BAGS - 2, DIM, rows, labels,
            "renyi:.9", "gaussian",
            &div_params, default_c_vals, num_default_c_vals,
            &default_svm_params,
            2, NULL);
    printf("Name: %s\n", SDM_ClassifyD_getName(sdm));

    printf("\n\nSingle predictions:\n");
    for (i = 0; i < NUM_BAGS; i++) {
        printf("%d: %d\n", i, SDM_ClassifyD_predict(sdm, data[i], rows[i]));
    }

    printf("\n\nSingle predictions, with decision values:\n");
    size_t num_vals;
    double *my_vals;
    int pred;
    for (i = 0; i < NUM_BAGS; i++) {
        pred = SDM_ClassifyD_predict_vals(
                sdm, data[i], rows[i], &my_vals, &num_vals);
        //SDM_ClassifyD_predict_many_vals(sdm, &data[i], 1, &rows[i],
        //        &pred, &dec_vals, &num_vals);

        printf("%d: %d   Vals: ", i, pred);
        for (j = 0; j < num_vals; j++)
            printf("%g ", my_vals[j]);
        printf("\n");

        free(my_vals);
    }

    printf("\n\nMass predictions, with decision values:\n");
    double **dec_vals;
    int *pred_labels = (int *) malloc(NUM_BAGS * sizeof(int));
    SDM_ClassifyD_predict_many_vals(sdm, data, NUM_BAGS, rows,
            pred_labels, &dec_vals, &num_vals);
    for (i = 0; i < NUM_BAGS; i++) {
        printf("%d: %d   Vals: ", i, pred_labels[i]);
        for (j = 0; j < num_vals; j++)
            printf("%g ", dec_vals[i][j]);
        printf("\n");
    }
    for (i = 0; i < NUM_BAGS; i++)
        free(dec_vals[i]);
    free(dec_vals);

    SDM_ClassifyD_freeModel(sdm);

    printf("\n\nTransduction on last 2: ");
    int trans_preds[2];
    SDM_ClassifyD_transduct(
            data, NUM_BAGS - 2, rows,
            data + NUM_BAGS - 2, 2, rows + NUM_BAGS - 2,
            DIM, labels,
            "renyi:.9", "gaussian", &div_params,
            default_c_vals, num_default_c_vals, &default_svm_params, 2,
            NULL, trans_preds);
    printf("%d (%d), %d (%d)\n",
            trans_preds[0], labels[NUM_BAGS - 2],
            trans_preds[1], labels[NUM_BAGS - 1]);


    div_params.show_progress = 5;
    div_params.print_progress = print_progress_to_stderr;

    printf("\n\nAbout to do regression, showing some progress output:\n");
    double rmse = sdm_crossvalidate_regress_double(
            data, NUM_BAGS, rows, DIM, means,
            "renyi:.9", "gaussian",
            &div_params, 2, 0, 1, 1,
            default_c_vals, num_default_c_vals, &default_svm_params, 2);
    printf("CV mean prediction RMSE: %g\n", rmse);

    printf("\n\nComputing divs:\n");
#define NUM_DFS 2
    const char * dfs[NUM_DFS] = {"renyi:.9", "hellinger"};
    double * divs[NUM_DFS];
    for (i = 0; i < NUM_DFS; i++)
        divs[i] = (double *) malloc(NUM_BAGS*NUM_BAGS * sizeof(double));
    np_divs_double(data, NUM_BAGS, rows, NULL, 0, NULL, DIM,
            dfs, 2, divs, &div_params);

    for (i = 0; i < NUM_DFS; i++)
        printf("CV with precomp %s, RMSE on means: %g\n", dfs[i],
            sdm_crossvalidate_regress_divs(
                divs[i], NUM_BAGS, means, "gaussian", 2, 0, 1, 1,
                default_c_vals, num_default_c_vals, &default_svm_params, 2)
        );

    int df;
    for (df = 0; df < NUM_DFS; df++) {
        printf("\n\nDivs (%s):\n", dfs[df]);
        for (i = 0; i < NUM_BAGS; i++) {
            for (j = 0; j < NUM_BAGS; j++) {
                printf("%7.2g ", divs[df][i*NUM_BAGS + j]);
            }
            printf("\n");
        }
    }
}
