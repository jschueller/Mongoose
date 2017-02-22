function comparisonData = compare(plot_outliers)
    index = UFget;
    j = 1;
    O = mongoose_getDefaultOptions();
    O.randomSeed = 123456789;
    comparisonData = struct('avg_mongoose_times', [], ...
                            'avg_metis_times', [], ...
                            'rel_mongoose_times',  [], ...
                            'rel_metis_times', [], ...
                            'avg_mongoose_imbalance', [], ...
                            'avg_metis_imbalance', [], ...
                            'rel_mongoose_imbalance', [], ...
                            'rel_metis_imbalance', [], ...
                            'avg_mongoose_cut_size', [], ...
                            'avg_metis_cut_size', [], ...
                            'rel_mongoose_cut_size', [], ...
                            'rel_metis_cut_size', [], ...
                            'problem_id', [], ...
                            'problem_name', [], ...
                            'problem_nnz', [], ...
                            'problem_n', []);
    for i = 1:1000%length(index.nrows)
        if (index.isReal(i))
            Prob = UFget(i);
            A = Prob.A;
            
            % If matrix is unsymmetric, form the augmented system
            if (index.numerical_symmetry(i) < 1)
                [m_rows, n_cols] = size(A);
                A = [sparse(m_rows,m_rows) A; A' sparse(n_cols,n_cols)];
            end
            
            % Sanitize the matrix (remove diagonal, take largest scc)
            A = mongoose_sanitizeMatrix(A);
            
            % If the sanitization removed all vertices, skip this matrix
            if nnz(A) < 2
                continue
            end
            
            fprintf('Computing separator for %d: %s\n', i, Prob.name);
            
            [~, n_cols] = size(A);
            comparisonData(j).problem_id = Prob.id;
            comparisonData(j).problem_name = Prob.name;
            comparisonData(j).problem_nnz = nnz(A);
            comparisonData(j).problem_n = n_cols;
            
            % Run Mongoose to partition the graph.
            for k = 1:5
                tic;
                partition = mongoose_computeEdgeSeparator(A,O);
                t = toc;
                fprintf('Mongoose: %0.2f\n', t);
                mongoose_times(j,k) = t;
                part_A = find(partition);
                part_B = find(1-partition);
                perm = [part_A part_B];
                p = length(partition);
                A_perm = A(perm, perm);
                mongoose_cut_size(j,k) = sum(sum(A_perm(p:n_cols, 1:p)));
                mongoose_imbalance(j,k) = abs(0.5-sum(partition)/length(partition));
            end
            
            % Run METIS to partition the graph.
            for k = 1:5
                tic;
                [perm,iperm] = metispart(A, 0, 123456789);
                t = toc;
                fprintf('METIS:    %0.2f\n', t);
                metis_times(j,k) = t;
                perm = [perm iperm];
                A_perm = A(perm, perm);
                metis_cut_size(j,k) = sum(sum(A_perm(p:n_cols, 1:p)));
                metis_imbalance(j,k) = abs(0.5-length(perm)/(length(perm)+length(iperm)));
            end
            j = j + 1;
        end
    end
    
    n = length(mongoose_times);
    
    for i = 1:n
        % Compute trimmed means - trim lowest and highest 20%
        comparisonData(i).avg_mongoose_times = trimmean(mongoose_times(i,:), 40);
        comparisonData(i).avg_mongoose_cut_size = trimmean(mongoose_cut_size(i,:), 40);
        comparisonData(i).avg_mongoose_imbalance = trimmean(mongoose_imbalance(i,:), 40);
        
        comparisonData(i).avg_metis_times = trimmean(metis_times(i,:), 40);
        comparisonData(i).avg_metis_cut_size = trimmean(metis_cut_size(i,:), 40);
        comparisonData(i).avg_metis_imbalance = trimmean(metis_imbalance(i,:), 40);
        
        % Compute times relative to METIS
        comparisonData(i).rel_mongoose_times = (comparisonData(i).avg_mongoose_times / comparisonData(i).avg_metis_times);
        
        % Compute cut size relative to METIS
        comparisonData(i).rel_mongoose_cut_size = (comparisonData(i).avg_mongoose_cut_size / comparisonData(i).avg_metis_cut_size);
        
        % Check for outliers
        prob_id = comparisonData(i).problem_id;
        outlier = false;
        
        if (comparisonData(i).rel_mongoose_times > 2)
            disp(['Outlier! Mongoose time significantly worse. ID: ', num2str(prob_id)]);
            outlier = true;
            comparisonData(i).outlier.time = 1;
        end
        if (comparisonData(i).rel_metis_times > 2)
            disp(['Outlier! METIS time significantly worse. ID: ', num2str(prob_id)]);
            outlier = true;
            comparisonData(i).outlier.time = -1;
        end
        
        if (comparisonData(i).rel_mongoose_cut_size > 100)
            disp(['Outlier! Mongoose cut size significantly worse. ID: ', num2str(prob_id)]);
            outlier = true;
            comparisonData(i).outlier.cut_size = 1;
        end
        if (comparisonData(i).rel_metis_cut_size > 100)
            disp(['Outlier! METIS cut size significantly worse. ID: ', num2str(prob_id)]);
            outlier = true;
            comparisonData(i).outlier.cut_size = -1;
        end
        
        if (comparisonData(i).avg_mongoose_imbalance > 2*comparisonData(i).avg_metis_imbalance)
            disp(['Outlier! Mongoose imbalance significantly worse. ID: ', num2str(prob_id)]);
            comparisonData(i).outlier.imbalance = 1;
            outlier = true;
        end
        if (comparisonData(i).avg_metis_imbalance > 2*comparisonData(i).avg_mongoose_imbalance)
            disp(['Outlier! METIS imbalance significantly worse. ID: ', num2str(prob_id)]);
            comparisonData(i).outlier.imbalance = -1;
            outlier = true;
        end
        
        if (outlier && plot_outliers)
            plotGraphs(prob_id);
        end
    end
    
    % Sort metrics
    sorted_rel_mongoose_times = sort([comparisonData.rel_mongoose_times]);
    sorted_rel_mongoose_cut_size = sort([comparisonData.rel_mongoose_cut_size]);
    sorted_avg_mongoose_imbalance = sort([comparisonData.avg_mongoose_imbalance]);
    sorted_avg_metis_imbalance = sort([comparisonData.avg_metis_imbalance]);
    
    % Get the Git commit hash for labeling purposes
    [error, commit] = system('git rev-parse --short HEAD');
    git_found = ~error;
    commit = strtrim(commit);
    
    %%%%% Plot performance profiles %%%%%
    
    % Plot timing profiles
    figure;
    semilogy(1:n, sorted_rel_mongoose_times, 'Color', 'b');
    hold on;
    semilogy(1:n, ones(1,n), 'Color','r');
    axis([1 n min(sorted_rel_mongoose_times) max(sorted_rel_mongoose_times)]);
    xlabel('Matrix');
    ylabel('Wall Time Relative to METIS');
    hold off;
    
    plt = Plot();
    plt.LineStyle = {'-', '--'};
    plt.Legend = {'Mongoose', 'METIS'};
    plt.LegendLoc = 'SouthEast';
    plt.BoxDim = [6, 5];
    
    filename = ['Timing' date];
    if(git_found)
        title(['Timing Profile - Commit ' commit]);
        filename = ['Timing-' commit];
    end
    
    plt.export([filename '.png']);
    
    % Plot separator size profiles
    figure;
    semilogy(1:n, sorted_rel_mongoose_cut_size, 'Color', 'b');
    hold on;
    semilogy(1:n, ones(1,n), 'Color','r');
    axis([1 n min(sorted_rel_mongoose_cut_size) max(sorted_rel_mongoose_cut_size)]);
    xlabel('Matrix');
    ylabel('Cut Size Relative to METIS');
    hold off;
    
    plt = Plot();
    plt.LineStyle = {'-', '--'};
    plt.Legend = {'Mongoose', 'METIS'};
    plt.LegendLoc = 'SouthEast';
    plt.BoxDim = [6, 5];
    
    filename = ['SeparatorSize' date];
    if(git_found)
        title(['Separator Size Profile - Commit ' commit]);
        filename = ['SeparatorSize-' commit];
    end
    plt.export([filename '.png']);
    
    % Plot imbalance profiles
    figure;
    plot(1:n, sorted_avg_mongoose_imbalance, 'Color', 'b');
    hold on;
    plot(1:n, sorted_avg_metis_imbalance, 'Color','r');
    axis([1 n 0 0.3]);
    xlabel('Matrix');
    ylabel('Imbalance');
    hold off;
    
    plt = Plot();
    plt.LineStyle = {'-', '--'};
    plt.Legend = {'Mongoose', 'METIS'};
    plt.BoxDim = [6, 5];
    
    filename = ['Imbalance' date];
    if(git_found)
        title(['Imbalance Profile - Commit ' commit]);
        filename = ['Imbalance-' commit];
    end
    
    plt.export([filename '.png']);
    
    % Write data to file for future comparisons
    if(git_found)
        writetable(struct2table(comparisonData), [commit '.txt']);
    end
end

function plotGraphs(prob_id)
    index = UFget;
    Prob = UFget(prob_id);
    A = Prob.A;
    if (index.numerical_symmetry(prob_id) < 1)
        [m_rows, n_cols] = size(A);
        A = [sparse(m_rows,m_rows) A; A' sparse(n_cols,n_cols)];
    end
    A = mongoose_sanitizeMatrix(A);
    
    % Compute partitioning using Mongoose
    partition = mongoose_computeEdgeSeparator(A);
%     part_A = find(partition);
%     part_B = find(1-partition);
%     perm = [part_A part_B];
%     p = length(partition);
%     A_perm = A(perm, perm);
%     subplot(1,2,1);
%     hold on;
%     spy(A);
%     subplot(1,2,2);
%     spy(A_perm);
%     hold off;
    mongoose_separator_plot(A, partition, 1-partition, ['mongoose_' num2str(prob_id)]);
    
    % Compute partitioning using METIS
    [perm, ~] = metispart(A, 0, 123456789);
    [m, ~] = size(A);
    partition = zeros(m,1);
    for j = 1:m
        partition(j,1) = sum(sign(find(j == perm)));
    end
    mongoose_separator_plot(A, partition, 1-partition, ['metis_' num2str(prob_id)]);
end

function plotMatrix(prob_id)
    index = UFget;
    Prob = UFget(prob_id);
    A = Prob.A;
    if (index.numerical_symmetry(prob_id) < 1)
        [m_rows, n_cols] = size(A);
        A = [sparse(m_rows,m_rows) A; A' sparse(n_cols,n_cols)];
    end
    A = mongoose_sanitizeMatrix(A);
    
    % Compute partitioning using Mongoose
    partition = mongoose_computeEdgeSeparator(A);
    part_A = find(partition);
    part_B = find(1-partition);
    perm = [part_A part_B];
    A_perm = A(perm, perm);
    subplot(1,2,1);
    hold on;
    spy(A);
    subplot(1,2,2);
    spy(A_perm);
    hold off;
end